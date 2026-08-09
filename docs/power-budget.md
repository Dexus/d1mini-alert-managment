# Power budget and battery runtime

This document gives planning values for USB-powered alert receivers. Battery runtime is an estimate and must be verified with the actual power bank, D1 mini board, LEDs, buzzer and Wi-Fi conditions.

## Why 10,000 mAh is not 10,000 mAh at 5 V

Most USB power banks quote capacity at the internal lithium cell voltage, typically around 3.7 V.

A nominal 10,000 mAh pack therefore contains approximately:

```text
10 Ah * 3.7 V = 37 Wh
```

The power bank then converts that energy to 5 V. Assuming approximately 85-90% usable conversion efficiency:

```text
usable energy ~= 31-33 Wh
```

Equivalent usable capacity at 5 V is therefore roughly:

```text
31 Wh / 5 V = 6.2 Ah
33 Wh / 5 V = 6.6 Ah
```

For conservative planning this project uses **6.2 Ah usable at 5 V** for a nominal 10,000 mAh power bank.

## Runtime formula

```text
runtime_hours = usable_5V_capacity_Ah / average_current_A
```

Using 6.2 Ah as the conservative usable capacity:

| Average 5 V current | Approx. runtime | Approx. days |
|---:|---:|---:|
| 70 mA | 88.6 h | 3.7 d |
| 80 mA | 77.5 h | 3.2 d |
| 90 mA | 68.9 h | 2.9 d |
| 100 mA | 62 h | 2.6 d |
| 120 mA | 51.7 h | 2.2 d |
| 150 mA | 41.3 h | 1.7 d |
| 200 mA | 31 h | 1.3 d |

## Expected receiver runtime

A receiver with Wi-Fi and MQTT continuously connected is currently budgeted at roughly 70-100 mA while idle.

Therefore a single receiver on a good 10,000 mAh power bank should be expected to run approximately **2.5 to 3.5 days in standby**.

This is intentionally a range. ESP8266 current depends on board revision, regulator losses, Wi-Fi signal quality, reconnect frequency and MQTT traffic.

## Alarm impact

The proposed receiver uses approximately:

- 70-80 mA for six blue LEDs when continuously on
- 20-40 mA for the optional active buzzer

A continuous full alarm can therefore raise total current into roughly the 160-220 mA range.

Short alarm periods have little effect on multi-day runtime. A receiver that remains in alarm for hours will discharge the battery substantially faster.

Example with 5 minutes of alarm per day:

```text
standby: 90 mA for 23 h 55 min
alarm:   190 mA for 5 min
```

The resulting daily average remains close to 90 mA, so the impact is small.

## Multiple receivers from one 10,000 mAh power bank

If several receivers share one power bank via a powered multi-port USB adapter or distribution hub, their current adds directly.

Assuming 90 mA average per receiver:

| Receivers | Total average current | Approx. runtime |
|---:|---:|---:|
| 1 | 90 mA | 68.9 h / 2.9 d |
| 2 | 180 mA | 34.4 h / 1.4 d |
| 3 | 270 mA | 23.0 h |
| 4 | 360 mA | 17.2 h |
| 6 | 540 mA | 11.5 h |
| 8 | 720 mA | 8.6 h |

A shared battery therefore makes sense for temporary operation, but separate power banks or mains USB power are preferable for long standby times.

## USB multi-port adapter requirements

For mains operation, use a regulated 5 V USB multi-port power adapter whose **total output rating** is high enough for all receivers simultaneously.

Use 0.25 A per receiver as a planning value and add at least 25% reserve.

Example for four receivers:

```text
4 * 0.25 A = 1.0 A
1.0 A * 1.25 = 1.25 A minimum planning requirement
```

A 5 V / 2 A adapter is therefore a sensible minimum for four receivers.

For eight receivers, use at least a quality 5 V / 3 A supply or larger.

## Power-bank auto-shutdown

Some power banks switch off when the load is below an internal threshold. One ESP8266 receiver will often draw enough current to avoid this, but this must not be assumed.

For battery-backed alarm receivers choose a power bank with one of these properties:

- documented always-on output
- documented low-current mode that stays enabled continuously
- verified in a multi-day test with the actual receiver

Do not treat a consumer power bank as validated until it has completed a long-duration test without shutting down.

## Acceptance test

Before field use, perform at least this power test:

1. Fully charge the 10,000 mAh power bank.
2. Connect the final receiver hardware and final USB cable.
3. Keep Wi-Fi and MQTT active continuously.
4. Trigger regular test alarms.
5. Record start and shutdown time.
6. Repeat with weak Wi-Fi conditions if that can occur at the installation location.

The measured runtime should replace the estimates in this document for the final hardware revision.
