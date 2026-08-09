# Receiver hardware

This document defines the first receiver hardware revision for the D1 mini alert system.

## Goals

- 5 V USB powered
- one 10,000 mAh USB power bank per receiver
- blue visual alarm
- optional 5 V active buzzer
- receiver must remain powered during normal charging / mains operation
- support both UPS/pass-through power banks and ordinary power banks without pass-through
- ESP8266 GPIOs must not directly source LED or buzzer current
- non-blocking firmware operation remains possible

## Recommended power architecture

There are now two supported supply variants.

### Variant A: power bank with verified UPS/pass-through

If the selected power bank has a verified UPS/pass-through mode and its 5 V output does not interrupt when the charger is connected/disconnected, its USB OUT may directly power the receiver.

See [`receiver-schematic-pass-through.svg`](receiver-schematic-pass-through.svg).

### Variant B: universal solution for power banks without pass-through

This is the preferred architecture when the power-bank behavior is unknown or pass-through is not supported.

A `TPS2113A` automatically selects between:

- IN1: wall USB 5 V, preferred source
- IN2: power-bank USB OUT 5 V

The wall 5 V source is split before the mux so that it also feeds the normal USB charge input of the power bank. Thus the receiver runs directly from mains while the battery charges; on mains loss the mux changes to power-bank output.

See:

- [`receiver-schematic-tps2113a.svg`](receiver-schematic-tps2113a.svg)
- [`power-mux-tps2113a.md`](power-mux-tps2113a.md)

Do not connect two USB 5 V outputs directly in parallel. The mux is the isolation and source-selection element.

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

The D1 mini and alarm outputs are supplied from the common `5V_SYS` rail.

For a simple verified UPS/pass-through power bank, `5V_SYS` may come directly from its USB output.

For the universal non-pass-through design, `5V_SYS` is the output of the TPS2113A power mux.

Recommended available current per receiver:

- minimum: 5 V / 500 mA
- preferred: 5 V / 1 A or more
- selected project power bank: 5 V / max. 2.1 A

The receiver normally consumes much less than 500 mA. The margin is intentional for ESP8266 Wi-Fi current peaks, LED load and buzzer load.

## Wall / multi-port USB power

Each receiver has its own power bank, but a common multi-port mains USB charger may feed several receiver/power-bank pairs.

For the universal TPS2113A version each receiver needs two 5 V branches from the mains source:

1. one branch to TPS2113A IN1 / receiver mains path
2. one branch to that receiver's power-bank USB charge input

This can be done with separate charger ports or a correctly rated 5 V distribution harness. Do not use a passive arrangement that accidentally ties power-bank OUT back into the charger.

The mains charger must be sized for the sum of:

- all receiver loads
- all simultaneous power-bank charge currents

Do not size it only from the receiver's ~0.25 A design budget.

## Local decoupling

Use local supply buffering close to the receiver electronics:

- 100 nF at each power-mux input
- at least 47-100 uF on `5V_SYS` after the mux
- 470-1000 uF electrolytic / low-ESR close to the D1 mini and alarm load
- 100 nF ceramic close to the D1 mini supply

This is especially useful with long or thin USB cables.

## Cable voltage drop

Cheap or long USB cables can cause more trouble than the power adapter itself. For alarm receivers use short, low-resistance cables. If the receiver resets while LEDs or buzzer are active, measure 5 V directly at the D1 mini under load and during source switchover.

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
- 1x TPS2113APW / TPS2113APWR for universal non-pass-through design
- 1x 680 ohm resistor for TPS2113A ILIM
- 1x 45.3 kohm + 1x 10 kohm for TPS2113A VSNS divider
- 2x 100 nF input decoupling for the power mux
- 1x 47-100 uF system capacitor after the mux
- 6x blue 5 mm LED
- 6x 150 ohm resistor, 0.25 W
- 2x AO3400A or another 3.3 V logic-level N-MOSFET
- 2x 100 ohm gate resistor
- 2x 10 kohm gate pulldown resistor
- 1x optional 5 V active buzzer
- 1x 470-1000 uF capacitor near receiver load
- 1x 100 nF ceramic capacitor near D1 mini
- USB cables / 5 V distribution with low resistance

## Required verification

Before fixing a PCB layout, measure one assembled receiver in these states:

1. Wi-Fi + MQTT connected, no alarm
2. LEDs continuously on
3. buzzer continuously on
4. LEDs + buzzer on while MQTT traffic is active
5. mains connected while power bank charges
6. mains removed: receiver must not reset
7. mains restored: receiver must not reset
8. repeat switchover after several hours of charging
9. repeat at low battery state of charge
10. verify that the power-bank USB output remains available or wakes automatically when mains disappears

Record average current, minimum `5V_SYS` voltage and whether MQTT remains connected through each transition.
