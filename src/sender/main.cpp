#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "config.h"

static constexpr uint8_t ALARM_PIN = D5;
static constexpr unsigned long DEBOUNCE_MS = 40;
static constexpr unsigned long WIFI_RETRY_MS = 2000;
static constexpr unsigned long MQTT_RETRY_MS = 1000;

AsyncMqttClient mqtt;

bool stableAlarm = false;
bool rawAlarm = false;
bool alarmActive = false;
bool mqttConnectPending = false;
unsigned long rawChangedAt = 0;
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastAlarmPublish = 0;
uint32_t alarmSequence = 0;
uint32_t bootNonce = 0;
String activeAlarmId;
bool receiverAcked[EXPECTED_RECEIVER_COUNT] = {};

String availabilityTopic() {
  return String("d1alert/sender/") + SENDER_DEVICE_ID + "/availability";
}

void clearAcks() {
  for (size_t i = 0; i < EXPECTED_RECEIVER_COUNT; ++i) receiverAcked[i] = false;
}

bool allReceiversAcked() {
  if (EXPECTED_RECEIVER_COUNT == 0) return true;
  for (size_t i = 0; i < EXPECTED_RECEIVER_COUNT; ++i) {
    if (!receiverAcked[i]) return false;
  }
  return true;
}

void markAck(const String& receiverId) {
  for (size_t i = 0; i < EXPECTED_RECEIVER_COUNT; ++i) {
    if (receiverId == EXPECTED_RECEIVERS[i]) {
      receiverAcked[i] = true;
      Serial.printf("ACK %s for %s\n", receiverId.c_str(), activeAlarmId.c_str());
      break;
    }
  }
}

String makeAlarmPayload() {
  char payload[224];
  snprintf(payload, sizeof(payload),
           "{\"state\":\"ALARM\",\"sender\":\"%s\",\"alarm_id\":\"%s\",\"uptime_ms\":%lu}",
           SENDER_DEVICE_ID, activeAlarmId.c_str(), millis());
  return String(payload);
}

void publishAlarm() {
  if (!alarmActive || !mqtt.connected()) return;

  const String payload = makeAlarmPayload();
  const uint16_t eventPacket = mqtt.publish(MQTT_TOPIC_ALARM, 1, false, payload.c_str());
  const uint16_t statePacket = mqtt.publish(MQTT_TOPIC_STATE, 1, true, payload.c_str());
  lastAlarmPublish = millis();

  Serial.printf("ALARM publish id=%s eventPacket=%u statePacket=%u\n",
                activeAlarmId.c_str(), eventPacket, statePacket);
}

void publishClear() {
  if (!mqtt.connected()) return;
  char payload[160];
  snprintf(payload, sizeof(payload),
           "{\"state\":\"CLEAR\",\"sender\":\"%s\",\"uptime_ms\":%lu}",
           SENDER_DEVICE_ID, millis());
  mqtt.publish(MQTT_TOPIC_STATE, 1, true, payload);
}

void startAlarm() {
  ++alarmSequence;
  activeAlarmId = String(SENDER_DEVICE_ID) + "-" + String(ESP.getChipId(), HEX) + "-" +
                  String(bootNonce, HEX) + "-" + String(alarmSequence);
  clearAcks();
  alarmActive = true;
  lastAlarmPublish = 0;
  publishAlarm();
}

void stopAlarm() {
  alarmActive = false;
  activeAlarmId = "";
  clearAcks();
  publishClear();
}

void parseAckTopic(const char* topic) {
  if (!alarmActive) return;

  const String prefix = String(MQTT_TOPIC_ACK_PREFIX) + "/";
  const String full(topic);
  if (!full.startsWith(prefix)) return;

  const String rest = full.substring(prefix.length());
  const int sep = rest.indexOf('/');
  if (sep <= 0) return;

  const String alarmId = rest.substring(0, sep);
  const String receiverId = rest.substring(sep + 1);
  if (alarmId != activeAlarmId || receiverId.length() == 0) return;

  markAck(receiverId);
  if (allReceiversAcked()) {
    Serial.printf("All receivers acknowledged alarm %s\n", activeAlarmId.c_str());
  }
}

void onMqttConnect(bool sessionPresent) {
  (void)sessionPresent;
  mqttConnectPending = false;
  Serial.println("MQTT connected");

  const String ackTopic = String(MQTT_TOPIC_ACK_PREFIX) + "/#";
  mqtt.subscribe(ackTopic.c_str(), 1);
  const String avail = availabilityTopic();
  mqtt.publish(avail.c_str(), 1, true, "online");

  if (alarmActive) publishAlarm();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  (void)reason;
  mqttConnectPending = false;
  Serial.println("MQTT disconnected");
}

void onMqttMessage(char* topic, char* payload,
                   AsyncMqttClientMessageProperties properties,
                   size_t len, size_t index, size_t total) {
  (void)payload;
  (void)properties;
  (void)len;
  (void)index;
  (void)total;
  parseAckTopic(topic);
}

void onMqttPublish(uint16_t packetId) {
  // For QoS 1 this callback is reached after PUBACK from the broker.
  Serial.printf("MQTT PUBACK packet=%u\n", packetId);
}

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  const unsigned long now = millis();
  if (now - lastWifiAttempt < WIFI_RETRY_MS) return;
  lastWifiAttempt = now;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void ensureMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected() || mqttConnectPending) return;
  const unsigned long now = millis();
  if (now - lastMqttAttempt < MQTT_RETRY_MS) return;
  lastMqttAttempt = now;
  mqttConnectPending = true;
  mqtt.connect();
}

void processAlarmInput() {
  const bool current = digitalRead(ALARM_PIN) == HIGH;
  const unsigned long now = millis();

  if (current != rawAlarm) {
    rawAlarm = current;
    rawChangedAt = now;
  }

  if (current != stableAlarm && now - rawChangedAt >= DEBOUNCE_MS) {
    stableAlarm = current;
    if (stableAlarm) startAlarm();
    else stopAlarm();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ALARM_PIN, INPUT); // External 10k pulldown required.

  randomSeed(ESP.getCycleCount() ^ micros());
  bootNonce = static_cast<uint32_t>(random(1, 0x7fffffff));

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  mqtt.onConnect(onMqttConnect);
  mqtt.onDisconnect(onMqttDisconnect);
  mqtt.onMessage(onMqttMessage);
  mqtt.onPublish(onMqttPublish);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setClientId((String("d1alert-sender-") + SENDER_DEVICE_ID).c_str());
  if (strlen(MQTT_USER) > 0) mqtt.setCredentials(MQTT_USER, MQTT_PASSWORD);

  const String avail = availabilityTopic();
  mqtt.setWill(avail.c_str(), 1, true, "offline");

  rawAlarm = digitalRead(ALARM_PIN) == HIGH;
  stableAlarm = rawAlarm;
  if (stableAlarm) startAlarm();
}

void loop() {
  ensureWifi();
  ensureMqtt();
  processAlarmInput();

  if (alarmActive && mqtt.connected() && !allReceiversAcked()) {
    const unsigned long now = millis();
    if (lastAlarmPublish == 0 || now - lastAlarmPublish >= ALARM_RETRY_MS) {
      publishAlarm();
    }
  }

  yield();
}
