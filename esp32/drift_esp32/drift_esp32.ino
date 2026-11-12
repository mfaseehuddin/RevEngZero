// ESP32 -> Zero Drift (Moyoung V2) watch messenger
// Library: NimBLE-Arduino 2.x (Library Manager: "NimBLE-Arduino" by h2zero)
// Serial Monitor at 115200, line ending "Newline":
//   type any text  -> shows as a message on the watch
//   buzz           -> makes the watch vibrate (find device)

#include <NimBLEDevice.h>
#include <vector>

static const char* WATCH_NAME = "DRIFT";          // advertised name from your scan
static const uint8_t MSG_TYPE  = 0x01;            // type byte that worked for you

static NimBLEUUID SVC_UUID((uint16_t)0xFEEA);     // Moyoung command service
static NimBLEUUID CMD_UUID((uint16_t)0xFEE2);     // write commands here
static NimBLEUUID RX1_UUID((uint16_t)0xFEE1);     // replies (notify)
static NimBLEUUID RX3_UUID((uint16_t)0xFEE3);     // replies (notify)

NimBLEClient* client = nullptr;
NimBLERemoteCharacteristic* cmdChr = nullptr;
unsigned long lastAttempt = 0;

void onNotify(NimBLERemoteCharacteristic* c, uint8_t* data, size_t len, bool) {
  Serial.printf("<- %s: ", c->getUUID().toString().c_str());
  for (size_t i = 0; i < len; i++) Serial.printf("%02X ", data[i]);
  Serial.println();
}

// Build FE EA [0x20+sizeHi] [sizeLo] [cmd] [payload...] and write it in MTU-sized chunks
bool sendPacket(uint8_t cmd, const uint8_t* payload, size_t plen) {
  if (!client || !client->isConnected() || !cmdChr) return false;

  size_t total = 5 + plen;
  std::vector<uint8_t> pkt;
  pkt.reserve(total);
  pkt.push_back(0xFE);
  pkt.push_back(0xEA);
  pkt.push_back(0x20 + ((total >> 8) & 0xFF));
  pkt.push_back(total & 0xFF);
  pkt.push_back(cmd);
  if (plen) pkt.insert(pkt.end(), payload, payload + plen);

  int mtu = client->getMTU();
  size_t chunk = (mtu > 23) ? (size_t)(mtu - 3) : 20;

  for (size_t off = 0; off < total; off += chunk) {
    size_t n = min(chunk, total - off);
    if (!cmdChr->writeValue(pkt.data() + off, n, false)) return false;
    delay(20);
  }
  return true;
}

bool sendMessage(const String& text) {
  std::vector<uint8_t> payload;
  payload.push_back(MSG_TYPE);
  for (size_t i = 0; i < text.length(); i++) payload.push_back((uint8_t)text[i]); // UTF-8 bytes
  return sendPacket(0x41, payload.data(), payload.size());
}

bool buzz() { return sendPacket(0x61, nullptr, 0); }

bool connectWatch() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  Serial.println("Scanning for watch...");
  NimBLEScanResults res = scan->getResults(5000, false);

  for (int i = 0; i < res.getCount(); i++) {
    const NimBLEAdvertisedDevice* d = res.getDevice(i);
    if (d->getName() != WATCH_NAME && !d->isAdvertisingService(SVC_UUID)) continue;

    Serial.printf("Found %s, connecting...\n", d->getAddress().toString().c_str());
    if (!client) client = NimBLEDevice::createClient();
    if (!client->connect(d)) { Serial.println("Connect failed"); return false; }

    NimBLERemoteService* svc = client->getService(SVC_UUID);
    if (!svc) { Serial.println("FEEA service missing"); client->disconnect(); return false; }

    cmdChr = svc->getCharacteristic(CMD_UUID);
    if (!cmdChr) { Serial.println("FEE2 missing"); client->disconnect(); return false; }

    NimBLERemoteCharacteristic* rx1 = svc->getCharacteristic(RX1_UUID);
    NimBLERemoteCharacteristic* rx3 = svc->getCharacteristic(RX3_UUID);
    if (rx1 && rx1->canNotify()) rx1->subscribe(true, onNotify);
    if (rx3 && rx3->canNotify()) rx3->subscribe(true, onNotify);

    Serial.printf("Connected (MTU %d). Type a message or 'buzz'.\n", client->getMTU());
    return true;
  }
  Serial.println("Watch not found (is the phone app still connected?)");
  return false;
}

void setup() {
  Serial.begin(115200);
  NimBLEDevice::init("");
  NimBLEDevice::setMTU(247);
}

void loop() {
  if (!client || !client->isConnected()) {
    cmdChr = nullptr;
    if (millis() - lastAttempt > 5000) {
      lastAttempt = millis();
      connectWatch();
    }
    return;
  }

  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;
    bool ok = line.equalsIgnoreCase("buzz") ? buzz() : sendMessage(line);
    Serial.println(ok ? "-> sent" : "-> send failed");
  }
}
