#pragma once

// Copy this file to include/config.h and adjust locally.
#define WIFI_SSID "your-wifi"
#define WIFI_PASSWORD "your-password"

#define MQTT_HOST "192.168.1.10"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""

#define SENDER_DEVICE_ID "melder-01"
#define RECEIVER_DEVICE_ID "receiver-01"

#define MQTT_TOPIC_ALARM "d1alert/alarm"
#define MQTT_TOPIC_STATE "d1alert/state"
#define MQTT_TOPIC_ACK_PREFIX "d1alert/ack"

// Sender waits for an application-level ACK from every configured receiver.
// Keep this list in sync with deployed receivers.
static const char* EXPECTED_RECEIVERS[] = {
  "receiver-01",
  "receiver-02"
};
static constexpr size_t EXPECTED_RECEIVER_COUNT =
    sizeof(EXPECTED_RECEIVERS) / sizeof(EXPECTED_RECEIVERS[0]);

// Retry active alarm until all configured receivers acknowledged it.
static constexpr unsigned long ALARM_RETRY_MS = 1500;

// Receiver output pins. D5/D6 are safe general-purpose GPIOs on D1 mini.
#define RECEIVER_LED_PIN D5
#define RECEIVER_BUZZER_PIN D6
#define RECEIVER_BUZZER_ENABLED 1
