# MQTT protocol

## Goals

The transport must not treat broker acceptance as proof that every physical receiver reacted. Therefore the protocol combines MQTT QoS 1 with an application-level acknowledgement from each receiver.

## Alarm event

Topic:

```text
d1alert/alarm
```

QoS: 1
Retained: no

Payload example:

```json
{
  "state": "ALARM",
  "sender": "melder-01",
  "alarm_id": "melder-01-8ac012-ab12cd34-17",
  "uptime_ms": 123456
}
```

`alarm_id` identifies one concrete activation of the Swissphone relay. Duplicate deliveries or sender retries use the same `alarm_id`.

## Retained state

Topic:

```text
d1alert/state
```

QoS: 1
Retained: yes

While the relay is active this contains the same ALARM document as the event topic. When the relay opens, the sender publishes:

```json
{
  "state": "CLEAR",
  "sender": "melder-01",
  "uptime_ms": 140000
}
```

This lets a receiver that reconnects after the original event recover the active alarm.

## Receiver acknowledgement

After the receiver has accepted the alarm and activated its local visual/acoustic output it publishes:

```text
d1alert/ack/<alarm_id>/<receiver_id>
```

Example:

```text
d1alert/ack/melder-01-8ac012-ab12cd34-17/receiver-01
```

QoS: 1
Retained: no
Payload: `ACK`

The sender subscribes to `d1alert/ack/#` with QoS 1 and records acknowledgements only when both the `alarm_id` and `receiver_id` match the current alarm and configured receiver list.

## Retry behavior

The sender republishes the same alarm every `ALARM_RETRY_MS` while at least one configured receiver has not acknowledged it. Receivers must therefore treat duplicate messages as idempotent: keep the alarm active and send the ACK again.

Once all expected receivers acknowledged the current alarm, periodic alarm retransmission stops. The retained ALARM state remains until the Swissphone relay opens.

## MQTT QoS vs application ACK

MQTT QoS 1 guarantees at-least-once transfer between a client and the broker and therefore permits duplicates. The `onPublish` callback is used as confirmation that the broker returned PUBACK for a QoS 1 publish.

That PUBACK does not prove that a receiver switched its local outputs. The separate application ACK supplies that second level of confirmation.

## Availability

Sender:

```text
d1alert/sender/<sender_id>/availability
```

Receiver:

```text
d1alert/receiver/<receiver_id>/availability
```

QoS: 1
Retained: yes

Normal payload: `online`
MQTT Last Will payload: `offline`
