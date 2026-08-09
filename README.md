# d1mini-alert-managment

MQTT based alert distribution using ESP8266 D1 mini devices.

The first implementation is a sender connected to the potential-free relay of a Swissphone LG s.QUAD charger. The sender remains connected to Wi-Fi/MQTT and broadcasts an alarm immediately when the relay closes.

## Build

This project uses PlatformIO.

1. Copy `include/config.example.h` to `include/config.h`.
2. Configure Wi-Fi, MQTT broker and device ID.
3. Build/upload:

```sh
pio run
pio run -t upload
```

`include/config.h` must contain credentials and must not be committed.

## MQTT

- `d1alert/alarm`: transient alarm event
- `d1alert/state`: retained current alarm state
- `d1alert/sender/<device>/availability`: retained online/offline state using MQTT Last Will

The retained state allows receivers that reconnect after the event to discover that an alarm is active.

## Hardware

See [`docs/sender-hardware.md`](docs/sender-hardware.md).

The Mini-DIN +5 V output is explicitly rated at max. 200 mA. Validate supply stability with the real ESP8266 hardware before production use.

## Status

Initial sender prototype. Receiver firmware and a stronger MQTT delivery/acknowledgement model are next steps.
