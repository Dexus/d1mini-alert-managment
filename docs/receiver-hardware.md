# Receiver hardware

This document defines the first receiver hardware revision for the D1 mini alert system.

## Goals

- 5 V USB powered
- compatible with a multi-port USB power adapter or power bank
- blue visual alarm
- optional 5 V active buzzer
- ESP8266 GPIOs must not directly source LED or buzzer current
- non-blocking firmware operation remains possible

## Recommended pin assignment

| Function | D1 mini pin | ESP8266 GPIO |
|---|---:|---:|
| Blue LEDs | D5 | GPIO14 |
| Buzzer | D6 | GPIO12 |

D5 and D6 are used because they are not ESP8266 boot-strap pins.

## Visual alarm

Recommended first revision: 6 blue LEDs, each with its own series resistor, switched by one logic-level N-MOSFET.

```text
USB +5 V
  |
  +-- 150R --|>|--+
  +-- 150R --|>|--+
  +-- 150R --|>|--+
  +-- 150R --|>|--+---- Drain Q1
  +-- 150R --|>|--+       AO3400A / equivalent
  +-- 150R --|>|--+---- Source -> GND

D5/GPIO14 -- 100R -- Gate Q1
                       |
                      10k
                       |
                      GND
```

For a typical blue LED forward voltage around 3.0-3.2 V, 150 ohm at 5 V produces roughly 12 mA per LED. Six LEDs therefore use approximately 70-80 mA when continuously on.

Each LED must have its own series resistor. Do not parallel LEDs behind one shared resistor.

## Optional buzzer

Use a 5 V active buzzer so the receiver only has to switch it on/off.

```text
USB +5 V ---- (+) active buzzer (-) ---- Drain Q2
                                           |
                                      AO3400A
                                           |
                                        Source
                                           |
                                          GND

D6/GPIO12 -- 100R -- Gate Q2
                       |
                      10k
                       |
                      GND
```

If an inductive magnetic buzzer is used instead of a piezo buzzer, add a flyback diode according to the buzzer type. For a piezo active buzzer a flyback diode is normally not required.

## Receiver supply

The receiver is supplied through the D1 mini USB connector. This keeps the 5 V input path simple and makes the receiver compatible with:

- normal USB wall adapters
- multi-port USB chargers
- USB power banks
- powered USB distribution hubs

Recommended supply capability per receiver:

- minimum: 5 V / 500 mA available per port
- preferred: 5 V / 1 A available per port

The receiver normally consumes much less than 500 mA. The margin is intentional for ESP8266 Wi-Fi current peaks, LED load and buzzer load.

## Multi-port USB power

For multiple receivers from one adapter, dimension the adapter for the sum of all possible receiver loads, not only their idle load.

Recommended planning value:

- 0.25 A per receiver for sizing
- plus at least 25% reserve

Examples:

| Receivers | Planning current | Recommended adapter |
|---:|---:|---:|
| 1 | 0.25 A | 5 V / 1 A or larger |
| 2 | 0.50 A | 5 V / 1 A or larger |
| 4 | 1.00 A | 5 V / 2 A or larger |
| 6 | 1.50 A | 5 V / 2.5-3 A or larger |
| 8 | 2.00 A | 5 V / 3 A or larger |

A passive USB splitter does not create additional power capacity. The upstream 5 V source must be rated for the total load.

Do not parallel multiple unrelated 5 V USB supplies into the same receiver or splitter. This can cause backfeeding between supplies.

## Local decoupling

Even when powered by a good USB adapter, add local supply buffering close to the receiver electronics:

- 470-1000 uF electrolytic or low-ESR capacitor between 5 V and GND
- 100 nF ceramic capacitor between 5 V and GND

This is especially useful with long or thin USB cables.

## Cable voltage drop

Cheap or long USB cables can cause more trouble than the power adapter itself. For alarm receivers use short, low-resistance cables. If the receiver resets while LEDs or buzzer are active, measure 5 V directly at the D1 mini under load.

## Approximate current budget

Typical design estimate for one receiver:

| Load | Approx. current |
|---|---:|
| D1 mini / ESP8266, Wi-Fi connected | 70-100 mA average |
| 6 blue LEDs | 70-80 mA when on |
| active buzzer | 20-40 mA when on |
| Total, idle | about 70-100 mA |
| Total, full continuous alarm | about 160-220 mA |

Actual hardware must be measured. The values above are engineering estimates for supply sizing, not guaranteed component specifications.

## Recommended prototype BOM

- 1x D1 mini ESP8266
- 6x blue 5 mm LED
- 6x 150 ohm resistor, 0.25 W
- 2x AO3400A or another 3.3 V logic-level N-MOSFET
- 2x 100 ohm gate resistor
- 2x 10 kohm gate pulldown resistor
- 1x optional 5 V active buzzer
- 1x 470-1000 uF capacitor
- 1x 100 nF ceramic capacitor
- USB cable with low resistance

## Next verification step

Before fixing a PCB layout, measure one assembled receiver in these states:

1. Wi-Fi + MQTT connected, no alarm
2. LEDs continuously on
3. buzzer continuously on
4. LEDs + buzzer on while MQTT traffic is active

Record both average current and minimum 5 V rail voltage at the D1 mini.
