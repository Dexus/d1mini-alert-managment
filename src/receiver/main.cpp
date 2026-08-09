#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "config.h"

static constexpr unsigned long WIFI_RETRY_MS = 2000;
static constexpr unsigned long MQTT_RETRY_MS = 1000;
static constexpr unsigned long SIGNAL_INTERVAL_MS = 250;

AsyncMqttClient mqtt;

bool mqttConnectPending = false;
bool alertActive = false;
bool signalState = false;
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastSignalToggle = 0;
String currentAlarmId;
String mqttClientId;
String availability;

String extractJsonString(const char* payload, size_t len, const char* key) {
  String body;
  body.reserve(len + 1);
  for (size_t i = 0; i < len; ++i) body += payload[i];

  const String marker = String("\"") + key + "\":\"";
  const int begin = body.indexOf(marker);
  if (begin < 0) return "";
  const int valueStart = begin + marker.length();
  const int valueEnd = body.indexOf('"', valueStart);
  if (valueEnd < 0) return "";
  return body.substring(valueStart, valueEnd);
}

void setOutputs(bool on) {
  digitalWrite(RECEIVER_LED_PIN, on ? HIGH : LOW);
#if RECEIVER_BUZZER_ENABLED
  digitalWrite(RECEIVER_BUZZER_PIN, on ? HIGH : LOW);
#endif
}

void acknowledgeAlarm(const String& alarmId) {
  if (!mqtt.connected() || alarmId.length() == 0) return;
  const String topic = String(MQTT_TOPIC_ACK_PREFIX) + "/" + alarmId + "/" + RECEIVER_DEVICE_ID;
  const uint16_t packetId = mqtt.publish(topic.c_str(), 1, false, "ACK");
  Serial.printf("ACK alarm=%s packet=%u\n", alarmId.c_str(), packetId);
}

void activateAlarm(const String& alarmId) {
  if (alarmId.length() == 0) return;
  currentAlarmId = alarmId;
  alertActive = true;
  signalState = true;
  lastSignalToggle = millis();
  setOutputs(true);
  acknowledgeAlarm(alarmId);
}

void clearAlarm() {
  alertActive = false;
  signalState = false;
  currentAlarmId = "";
  setOutputs(false);
}

void onMqttConnect(bool sessionPresent) {
  (void)sessionPresent;
  mqttConnectPending = false;
  Serial.println("MQTT connected");
  mqtt.subscribe(MQTT_TOPIC_ALARM, 1);
  mqtt.subscribe(MQTT_TOPIC_STATE, 1);
  mqtt.publish(availability.c_str(), 1, true, "online");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  (void)reason;
  mqttConnectPending = false;
  Serial.println("MQTT disconnected");
}

void onMqttMessage(char* topic, char* payload,
                   AsyncMqttClientMessageProperties properties,
                   size_t len, size_t index, size_t total) {
  (void)properties;
  if (index != 0 || len != total) return; // Alarm messages are deliberately small.

  const String state = extractJsonString(payload, len, "state");
  if (state == "ALARM") {
    const String alarmId = extractJsonString(payload, len, "alarm_id");
    activateAlarm(alarmId);
    Serial.printf("ALARM received via %s id=%s\n", topic, alarmId.c_str());
  } else if (state == "CLEAR" && String(topic) == MQTT_TOPIC_STATE) {
    clearAlarm();
    Serial.println("Alarm cleared");
  }
}

void onMqttPublish(uint16_t packetId) {
  // For QoS 1, this is the broker PUBACK for our application ACK/availability publish.
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

void updateSignal() {
  if (!alertActive) return;
  const unsigned long now = millis();
  if (now - lastSignalToggle < SIGNAL_INTERVAL_MS) return;
  lastSignalToggle = now;
  signalState = !signalState;
  setOutputs(signalState);
}

void setup() {
  Serial.begin(115200);
  pinMode(RECEIVER_LED_PIN, OUTPUT);
#if RECEIVER_BUZZER_ENABLED
  pinMode(RECEIVER_BUZZER_PIN, OUTPUT);
#endif
  setOutputs(false);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  mqttClientId = String("d1alert-receiver-") + RECEIVER_DEVICE_ID;
  availability = String("d1alert/receiver/") + RECEIVER_DEVICE_ID + "/availability";

  mqtt.onConnect(onMqttConnect);
  mqtt.onDisconnect(onMqttDisconnect);
  mqtt.onMessage(onMqttMessage);
  mqtt.onPublish(onMqttPublish);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setClientId(mqttClientId.c_str());
  if (strlen(MQTT_USER) > 0) mqtt.setCredentials(MQTT_USER, MQTT_PASSWORD);
  mqtt.setWill(availability.c_str(), 1, true, "offline");
}

void loop() {
  ensureWifi();
  ensureMqtt();
  updateSignal();
  yield();
}
