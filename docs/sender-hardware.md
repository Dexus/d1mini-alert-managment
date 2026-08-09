# Swissphone LG s.QUAD sender hardware

## Mini-DIN pinout

According to the LG s.QUAD documentation:

| Pin | Function |
|---|---|
| 1 | Relay Normally Open |
| 2 | GND |
| 3 | Relay Common |
| 4 | +5 V DC, max. 200 mA |
| 5 | not connected |
| 6 | Switched Voltage (+5 V DC) |

Relay contact rating: 30 V / 1 A AC/DC.

## Recommended alarm input

Use the potential-free relay rather than feeding the switched 5 V signal into an ESP8266 GPIO.

```
D1 mini 3V3 -------- Mini-DIN Pin 3 (COM)
                       relay
D1 mini D5/GPIO14 --- Mini-DIN Pin 1 (NO)
        |
       10k
        |
       GND

Mini-DIN Pin 2 ------ D1 mini GND
Mini-DIN Pin 4 ------ D1 mini 5V
```

When the Swissphone relay closes, D5 is driven to 3.3 V. When open, the external 10 kOhm resistor holds D5 low.

Do not connect Mini-DIN Pin 6 directly to an ESP8266 GPIO: it is specified as +5 V when switched.

## Supply warning

Mini-DIN Pin 4 is specified for a maximum load of 200 mA. ESP8266 Wi-Fi current has short peaks that can exceed the average current substantially. This must be validated on the actual charger before treating the Mini-DIN supply as production-safe.

For the prototype, place local bulk and high-frequency decoupling close to the D1 mini (for example 1000 uF low-ESR plus 100 nF). This improves transient behavior but does not change the charger's 200 mA output rating.

If the 5 V rail drops or the ESP8266 resets during association/transmission, use a separately dimensioned buffered supply rather than exceeding the Mini-DIN rating.
