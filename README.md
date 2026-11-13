# RevEngZero

Talking to a Zero "Drift" smartwatch directly over Bluetooth LE.

The Drift is a rebranded Moyoung / Da Fit watch (`MOYOUNG-V2`, firmware `MOY-LCU3-2.0.3`) on a Realtek BLE chip. Its command service (`0xFEEA`) works without pairing.

## Web app (`index.html`)

Open https://mfaseehuddin.github.io/RevEngZero/ in **Bluefy** (iPhone) or **Chrome** (Android/desktop), close the Zero app, and tap Connect.

Messages, buzz, watch faces, settings, heart rate, battery, queries and a raw packet box (risky commands blocked by default).

## ESP32 (`esp32/drift_esp32`)

Arduino sketch using NimBLE-Arduino 2.x. Type in the Serial Monitor (115200, Newline) to send a message, or `buzz` to vibrate.

## Protocol notes

Write packets to `0xFEE2`; replies notify on `0xFEE1` / `0xFEE3`.

```
FE EA  20+sizeHi  sizeLo  CMD  PAYLOAD...
```

Size counts every byte. The watch reports its face size in the reply to `39`: `… 00 F0 00 8C 01 18 00 A3 FF` = 240 wide, 140 preview wide, 280 high, 163 preview high.

Confirmed working:

| Packet | Effect |
|---|---|
| `FE EA 20 05 61` | Vibrate (find device) |
| `FE EA 20 08 41 01 48 69` | Message "Hi" (type 01) |
| `FE EA 20 08 41 08 48 69` | Message "Hi" with WhatsApp icon (type 08) |

Avoid `51` (shutdown), `63` (firmware upgrade), `6E` / `74` (file transfer).

References: [Gadgetbridge Moyoung protocol](https://gadgetbridge.org/internals/specifics/moyoung-protocol/), [uwatch2 notes](https://gist.github.com/kabbi/854a541c1a32e15fb0dfa3338f4ee4a9).

## Custom face upload (confirmed working)

The watch reports its face size in the reply to `39`; on the Drift it is **240×280**, preview **140×163** (not the 240×296 on spec sheets).

1. `FE EA 20 09 6E` + total size (4 bytes, big-endian) to `FEE2`
2. **Wait ~3 s**: the watch erases flash and silently drops anything sent sooner
3. Image data to `FEE6` (write without response): background 240×280 then preview 140×163, RGB565 big-endian, row by row
   - **128-byte chunks, 80 ms apart**: faster and the watch drops packets
   - **A short pause every 4 KB** (40 ms confirmed working): the watch can drop a packet while flushing each block to flash
4. `FE EA 20 09 6E 00 00 00 00` (finish), `FE EA 20 05 29`, `FE EA 20 06 19 01` (show photo face)

**Update:** the watch *does* acknowledge. On `FEE3` it replies `6E 00 00` when ready (about 2.3 s after start), then `6E nn nn` with the running count of packets received after every `FEE6` write, and finally `6E FF FF cc cc`: the watch reads the image back from flash and returns its CRC-16 (seeded `0xFEEA`, same as kabbi's notes), or `00 00` if verification failed. At ~9.7 KB/s (244-byte packets, 4 in flight) every packet was acknowledged but the stored copy was corrupted, so the real limit is flash write speed, not Bluetooth. Pacing on these replies replaces the fixed delays. A dropped packet shows up as the count falling behind, and the checksum confirms the stored image. A test pattern (colour bars, border, diagonal) makes each failure type easy to recognise.

## Clock layout (photo face)

`39` reads and `38` sets: position (`0` top, `1` bottom), content above and below (`0` none, `1` date, `2` sleep, `3` heart rate, `4` steps), clock color (RGB565, big-endian), a 32-byte background id, then on V2 the size block. The page echoes the watch's own bytes back with only those fields changed.

## Watch → phone events

| Cmd | Meaning |
|---|---|
| `67 xx` | Music/call button: `00` play/pause, `01` previous, `02` next, `03` reject call |
| `66` | Camera shutter pressed |
| `62 00` / `62 xx` | Find phone start / stop (send `62 FF` to stop from the phone) |
| `64` | Watch wants weather resent |
| `34` | Workout finished |

## Weather

- `43` today: `00`, condition, temperature (int8), 4 chars UTF-16BE (lunar/festival, spaces), 4 chars UTF-16BE (city)
- `42` forecast: 7 × (condition, low, high)
- Conditions: `0` cloudy, `1` fog, `2` overcast, `3` rain, `4` snow, `5` sunny, `6` wind, `7` haze

Protocol details from Gadgetbridge's Da Fit notes (DaFitConstants.java in the MT863 fork).

## Message icon types (command 41)

| Type | Icon |
|---|---|
| `00` | Incoming call |
| `01` | Message / SMS |
| `02` | WeChat |
| `03` | QQ |
| `04` | Facebook |
| `05` | Twitter / X |
| `06` | Instagram |
| `07` | Skype |
| `08` | WhatsApp |
| `09` | LINE |
| `0A` | KakaoTalk |
| `0B` | Gmail |
| `0C`–`1F` | Other (generic icon) |

## Versioning

The page shows its version under the title (`APP_VERSION` in `index.html`). Bump it on every commit so you can see when GitHub Pages has deployed the new build.

| Version | Change |
|---|---|
| 0.10.2 | Watch checksum confirmed as CRC-16 seeded 0xFEEA; a mismatch (or 0000) now cancels the upload, keeps the old face and retries at Safe speed |
| 0.10.1 | Correct message when the watch counts every packet but never confirms the file (data lost while saving); wait up to 8 s for confirmation |
| 0.10.0 | Ack-driven face upload: waits for the watch's ready signal, paces on its per-packet acknowledgements, verifies the final checksum |
| 0.9.2 | Upload trace recorder (every watch reply during a face upload with bytes sent and time) and Copy log |
| 0.9.1 | Upload speed presets with time estimates and remembered settings; default 4 KB pause 40 ms (133 s confirmed at 128 B / 80 ms) |
| 0.9.0 | Clock layout editor for the photo face (`38`/`39`): position, color, content above/below, live preview overlay |
| 0.8.0 | Watch events panel (music buttons, camera shutter, find phone rings the page, weather requests, unknown packets); live weather push via Open-Meteo; step goal set back to big-endian (query reply is little-endian) |
| 0.7.1 | 500 ms pause every 4 KB during face upload (watch drops a packet while flushing to flash) |
| 0.7.0 | Face upload fixed: 128-byte chunks at 80 ms (watch drops packets if faster) and a 3 s wait after the start command (watch drops the first packets while erasing flash) |
| 0.6.3 | Types 0C–1F labelled Other |
| 0.6.2 | Message icon types 02–0B labelled |
| 0.6.1 | Step goal is little-endian (was sent backwards) |
| 0.6.0 | Face size read from the watch (cmd 39 → 240×280, preview 140×163) instead of the spec-sheet 240×296; test pattern; 30 ms default chunk delay |
| 0.5.0 | Custom face designer and uploader (Mirage Horizon / Bold / your own image, 240×296 RGB565 via FEE6); Dump-for-Claude report; message icon tester (08 = WhatsApp) |
| 0.4.0 | Version label; Bluefy connect fix (full UUIDs, single picker call, readable errors); diagnostics and nearby-device scan |
| 0.1.0 | First web app, ESP32 sketch, protocol notes |
