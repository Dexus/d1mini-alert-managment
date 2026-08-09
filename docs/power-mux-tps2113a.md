# Universal 5 V power path with TPS2113A

This design allows a receiver to use a normal USB power bank even when the power bank does **not** support pass-through/UPS operation.

## Goal

- receiver remains powered while mains USB power is present
- power bank charges independently from the same mains 5 V source
- when mains 5 V disappears, the receiver automatically switches to the power-bank USB output
- no backfeed between wall adapter and power-bank output
- no manual cable swapping during normal operation

One power bank is used per receiver.

## Architecture

```text
                         5 V USB wall supply
                               |
                  +------------+-------------+
                  |                          |
                  |                          +----> Power bank USB IN
                  |                                (charge input)
                  |
                  +----> TPS2113A IN1
                               |
Power bank USB OUT ------------+----> TPS2113A IN2
                                    |
                                    +----> OUT / 5V_SYS
                                             |
                                      +------+------+
                                      |      |      |
                                   D1 mini  LEDs  buzzer
```

The mains source therefore has two jobs while available:

1. supply the receiver through TPS2113A IN1
2. charge the power bank through the power bank's normal USB input

The power bank output is connected only to TPS2113A IN2.

Do **not** connect wall-adapter 5 V and power-bank output together directly.

## TPS2113A wiring

Recommended TSSOP-8 part: `TPS2113APW` / `TPS2113APWR`.

| Pin | Name | Connection |
|---:|---|---|
| 1 | STAT | optional status output, otherwise leave unused |
| 2 | EN | GND (device enabled) |
| 3 | VSNS | divider from wall-adapter 5 V (IN1) |
| 4 | ILIM | 680 ohm to GND |
| 5 | GND | common GND |
| 6 | IN2 | power-bank USB OUT +5 V |
| 7 | OUT | system 5 V rail |
| 8 | IN1 | wall-adapter +5 V |

Place 100 nF from IN1 to GND and 100 nF from IN2 to GND close to the IC.

Use at least 47 uF on `5V_SYS`; the receiver board may additionally use 470-1000 uF close to the D1 mini for Wi-Fi/load transients.

## Mains priority threshold

The TPS2113A connects OUT to IN1 when VSNS is above about 0.8 V. If VSNS is below that threshold it selects the higher available input.

Use this divider from wall-adapter IN1 to VSNS:

```text
IN1 / wall 5 V
      |
    45.3k
      |
      +------ VSNS
      |
     10k
      |
     GND
```

Nominal changeover threshold:

```text
VIN1 ~= 0.8 V * (45.3k + 10k) / 10k
     ~= 4.42 V
```

So normal 5 V mains is preferred. If the wall source disappears or falls below roughly 4.4 V, the power-bank output on IN2 takes over.

The exact threshold includes the TPS2113A VSNS tolerance and hysteresis; this is a design target, not a precision 4.420 V supervisor.

## Current limit

TPS2113A nominal current limit is approximately:

```text
I_LIMIT ~= 500 / R_ILIM
```

with R in ohms and current in amperes.

For `R_ILIM = 680 ohm`:

```text
I_LIMIT ~= 0.74 A nominal
```

This is comfortably above the receiver's expected full-alarm load (~0.16-0.22 A) while still providing a useful protection limit.

The D1 mini, LEDs and buzzer must still be designed for their own normal currents; this limiter is not a substitute for LED resistors or output protection.

## Power-bank requirements

The power bank itself may be a normal consumer USB power bank without pass-through support.

Required behavior:

- USB OUT: 5 V, up to at least 0.5 A continuously; the selected project power bank provides up to 2.1 A
- USB output must remain enabled at the receiver's standby current
- charging input and output are electrically separate connectors as normal for a power bank

Because the receiver is normally supplied from the wall path, low-load auto-shutdown of the power-bank output still has to be tested. Some power banks may turn their output off while sitting unused and may not automatically re-enable it when mains disappears. Such a power bank is **not suitable** for this universal UPS arrangement unless its output can be kept permanently active.

This is the remaining behavioral dependency on the selected power bank.

## Wall USB supply sizing

While mains is present, the wall supply may simultaneously:

- run one receiver (~0.1 A standby, up to roughly 0.25 A design budget)
- charge that receiver's 10,000 mAh power bank

Therefore do not size the wall adapter only for the receiver. Size it for the power-bank charge current plus at least 0.25 A receiver reserve.

If a multi-port USB charger supplies several receiver/power-bank pairs, use one power-bank charge path and one receiver/mux path per receiver and ensure the charger's total shared current rating covers all ports simultaneously.

## Required acceptance test

For every selected power-bank model perform all of these tests with the final receiver:

1. power-bank output on, wall power absent: receiver must stay online
2. connect wall power: receiver must not reset
3. charge power bank for several hours while receiver remains online
4. remove wall power: receiver must not reset and MQTT must remain connected or reconnect immediately
5. repeat removal after the power bank has been sitting on charge for several hours
6. reconnect wall power: receiver must not reset
7. repeat at low power-bank state of charge
8. repeat while LEDs and buzzer are active

A power bank that disables its USB output while charging or while idle is unsuitable unless testing proves that the output is available immediately when IN1 disappears.

## Source

Component behavior and limits are based on the Texas Instruments TPS2113A data sheet. Keep the data sheet as the authority for PCB design, tolerances and absolute maximum ratings.
