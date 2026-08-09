# Power budget and battery runtime

This document gives planning values for USB-powered alert receivers. Battery runtime is an estimate and must be verified with the actual power bank, D1 mini board, LEDs, buzzer and Wi-Fi conditions.

## Power topology

Each receiver uses its **own dedicated 10,000 mAh power bank**. Receivers do not share one battery.

The selected power bank can provide up to **5 V / 2.1 A** on its USB output. This is far above the expected current requirement of one receiver and therefore provides generous peak-current headroom for ESP8266 Wi-Fi activity, LEDs and the optional buzzer.

The receiver is still designed around a conservative maximum planning current of approximately **0.25 A**. Compared with the 2.1 A source limit, this leaves more than enough reserve:

```text
2.1 A source capability / 0.25 A receiver design budget = 8.4x headroom
```

The 2.1 A figure is a source capability, not an expected receiver draw. The receiver should normally consume much less.

## USB connection

Power the D1 mini through its normal 5 V USB connector. A USB multi-adapter may be used mechanically to adapt connector types or cable arrangements, but it must not combine multiple receivers onto one battery output in this design.

Use a short, good-quality USB cable. Excessive cable resistance can cause voltage drop during ESP8266 Wi-Fi current peaks even when the power bank itself is capable of 2.1 A.

Local decoupling close to the receiver electronics remains recommended, for example:

- 470-1000 uF electrolytic or low-ESR capacitor across 5 V/GND
- 100 nF ceramic capacitor close to the switching/output electronics

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

Therefore one receiver on its dedicated 10,000 mAh power bank should be expected to run approximately **2.5 to 3.5 days in standby**.

This is intentionally a range. ESP8266 current depends on board revision, regulator losses, Wi-Fi signal quality, reconnect frequency and MQTT traffic.

## Alarm impact

The proposed receiver uses approximately:

- 70-80 mA for six blue LEDs when continuously on
- 20-40 mA for the optional active buzzer

A continuous full alarm can therefore raise total current into roughly the 160-220 mA range. This is still only a small fraction of the power bank's 2.1 A output capability.

Short alarm periods have little effect on multi-day runtime. A receiver that remains in alarm for hours will discharge the battery substantially faster.

Example with 5 minutes of alarm per day:

```text
standby: 90 mA for 23 h 55 min
alarm:   190 mA for 5 min
```

The resulting daily average remains close to 90 mA, so the impact is small.

## Power-bank auto-shutdown

Some power banks switch off when the load is below an internal threshold. One ESP8266 receiver will often draw enough current to avoid this, but this must not be assumed.

For battery-backed alarm receivers choose a power bank with one of these properties:

- documented always-on output
- documented low-current mode that stays enabled continuously
- verified in a multi-day test with the actual receiver

Do not treat a consumer power bank as validated until it has completed a long-duration test without shutting down.

## Acceptance test

Before field use, perform at least this power test for every receiver/power-bank combination:

1. Fully charge the 10,000 mAh power bank.
2. Connect exactly one final receiver using the final USB cable/adapters.
3. Keep Wi-Fi and MQTT active continuously.
4. Trigger regular test alarms with LEDs and buzzer enabled.
5. Record start and shutdown time.
6. Repeat with weak Wi-Fi conditions if that can occur at the installation location.
7. Confirm the power bank never enters automatic shutdown while the receiver is idle.

The measured runtime should replace the estimates in this document for the final hardware revision.
