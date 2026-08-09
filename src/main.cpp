#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config.h"

// Swissphone LG s.QUAD relay wiring:
// Pin 4 -> D1 mini 5V (max. 200 mA; see hardware documentation)
// Pin 2 -> D1 mini GND
// D1 mini 3V3 -> Mini-DIN Pin 3 (relay common)
// Mini-DIN Pin 1 (relay NO) -> D5/GPIO14
// 10k pulldown from D5/GPIO14 to GND.
static constexpr uint8_t ALARM_PIN = D5;
static constexpr unsigned long DEBOUNCE_MS = 40;
static constexpr unsigned long WIFI_RETRY_MS = 2000;
static constexpr unsigned long MQTT_RETRY_MS = 1000;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool stableAlarm = false;
bool rawAlarm = false;
bool pendingAlarm = false;
unsigned long rawChangedAt = 0;
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;
uint32_t alarmSequence = 0;

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  const unsigned long now = millis();
  if (now - lastWifiAttempt < WIFI_RETRY_MS) return;
  lastWifiAttempt = now;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void ensureMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) return;
  const unsigned long now = millis();
  if (now - lastMqttAttempt < MQTT_RETRY_MS) return;
  lastMqttAttempt = now;

  String clientId = String("d1alert-sender-") + DEVICE_ID + "-" + String(ESP.getChipId(), HEX);
  if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD,
                   MQTT_TOPIC_AVAILABILITY, 1, true, "offline")) {
    mqtt.publish(MQTT_TOPIC_AVAILABILITY, "online", true);
  }
}

void publishAlarm() {
  if (!mqtt.connected()) {
    pendingAlarm = true;
    return;
  }

  ++alarmSequence;
  char payload[192];
  snprintf(payload, sizeof(payload),
           "{\"state\":\"ALARM\",\"sender\":\"%s\",\"alarm_id\":\"%s-%lu\",\"uptime_ms\":%lu}",
           DEVICE_ID, DEVICE_ID, static_cast<unsigned long>(alarmSequence), millis());

  // PubSubClient publishes QoS 0. The retained state below prevents a newly
  // connected receiver from missing an active alarm. QoS 1 requires a client
  // library with outbound QoS 1 support and is planned for the next revision.
  const bool eventOk = mqtt.publish(MQTT_TOPIC_ALARM, payload, false);
  const bool stateOk = mqtt.publish(MQTT_TOPIC_STATE, payload, true);
  pendingAlarm = !(eventOk && stateOk);
}

void publishClear() {
  if (!mqtt.connected()) return;
  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"state\":\"CLEAR\",\"sender\":\"%s\",\"uptime_ms\":%lu}",
           DEVICE_ID, millis());
  mqtt.publish(MQTT_TOPIC_STATE, payload, true);
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
    if (stableAlarm) {
      pendingAlarm = true;
      publishAlarm();
    } else {
      publishClear();
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ALARM_PIN, INPUT); // External 10k pulldown required.

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);

  rawAlarm = digitalRead(ALARM_PIN) == HIGH;
  stableAlarm = rawAlarm;
  if (stableAlarm) pendingAlarm = true;
}

void loop() {
  ensureWifi();
  ensureMqtt();

  if (mqtt.connected()) {
    mqtt.loop();
    if (pendingAlarm) publishAlarm();
  }

  processAlarmInput();
  yield();
}
