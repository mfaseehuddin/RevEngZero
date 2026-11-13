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

## Versioning

The page shows its version under the title (`APP_VERSION` in `index.html`). Bump it on every commit so you can see when GitHub Pages has deployed the new build.

| Version | Change |
|---|---|
| 0.6.2 | Message icon types 02–0B labelled |
| 0.6.1 | Step goal is little-endian (was sent backwards) |
| 0.6.0 | Face size read from the watch (cmd 39 → 240×280, preview 140×163) instead of the spec-sheet 240×296; test pattern; 30 ms default chunk delay |
| 0.5.0 | Custom face designer and uploader (Mirage Horizon / Bold / your own image, 240×296 RGB565 via FEE6); Dump-for-Claude report; message icon tester (08 = WhatsApp) |
| 0.4.0 | Version label; Bluefy connect fix (full UUIDs, single picker call, readable errors); diagnostics and nearby-device scan |
| 0.1.0 | First web app, ESP32 sketch, protocol notes |
