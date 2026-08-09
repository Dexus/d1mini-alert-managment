# d1mini-alert-managment

MQTT based alert distribution using ESP8266 D1 mini devices.

The sender is connected to the potential-free relay of a Swissphone LG s.QUAD charger. It remains connected to Wi-Fi/MQTT and broadcasts an alarm immediately when the relay closes.

## Build

This project uses PlatformIO.

1. Copy `include/config.example.h` to `include/config.h`.
2. Configure Wi-Fi, MQTT broker, sender/receiver IDs and `EXPECTED_RECEIVERS`.
3. Build/upload the required role:

```sh
pio run -e sender
pio run -e sender -t upload

pio run -e receiver
pio run -e receiver -t upload
```

`include/config.h` contains credentials and is ignored by Git.

GitHub Actions builds both environments on every push and pull request.

## Delivery model

The project uses two reliability layers:

1. MQTT QoS 1 for alarm, state and ACK publishes. The broker confirms delivery to the publishing client with PUBACK.
2. Application-level receiver ACK. Every configured receiver acknowledges a concrete `alarm_id` only after it has accepted and activated the alarm locally.

The sender retries the active alarm every `ALARM_RETRY_MS` until every receiver listed in `EXPECTED_RECEIVERS` has acknowledged that `alarm_id`.

## MQTT topics

- `d1alert/alarm` - transient alarm event, QoS 1
- `d1alert/state` - retained current alarm state, QoS 1
- `d1alert/ack/<alarm_id>/<receiver_id>` - receiver application ACK, QoS 1
- `d1alert/sender/<device>/availability` - sender Last Will / online state
- `d1alert/receiver/<device>/availability` - receiver Last Will / online state

The retained alarm state allows a receiver that reconnects after the original event to discover the active alarm and acknowledge it.

See [`docs/mqtt-protocol.md`](docs/mqtt-protocol.md) for the protocol details.

## Hardware

Sender wiring: [`docs/sender-hardware.md`](docs/sender-hardware.md).

The Mini-DIN +5 V output is explicitly rated at max. 200 mA. Validate supply stability with the real ESP8266 hardware before production use.

The initial receiver firmware uses D5/GPIO14 for the visual output and D6/GPIO12 for the optional buzzer. Both outputs are intended to drive external MOSFET stages, not LED/buzzer loads directly.
