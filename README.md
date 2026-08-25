# Multi-Zone Reaction Trainer

A reflex-training device that tests more than just how fast you react — it
tests whether you react in the *right place*. Four ultrasonic sensors,
mounted in a fixed row, each watch their own zone; a laptop dashboard picks
one at random each round, times your response, and automatically corrects
for the communication link's own delay so that delay is never mistaken for
part of your reflex.

Built for the **Canara Vikas** competition, 2026.

## Why spatial reaction, not just speed

Most simple reaction testers measure one thing: a light turns on, you press
a button, the system times it. That tests speed alone. Real reactions are
rarely that simple — reacting quickly in the *wrong* direction is still a
failure. This project splits the test surface into four independent zones,
so a correct response has to be both fast **and** in the right place.

## How it works

- **Four ultrasonic sensors** (HC-SR04) sit in a fixed horizontal row,
  15 cm apart — Zone 1 through Zone 4, left to right.
- **An Arduino Uno** continuously reads all four sensors and streams their
  live distances over a serial link. It has no awareness of rounds,
  scoring, or difficulty — its only job is sensing and communication.
- **A browser dashboard** (`dashboard/index.html`) owns all of the game
  logic: picking the target zone, timing the round, checking whether the
  correct zone responded, scoring, and keeping session history — all
  running client-side, no server required.
- **Automatic latency calibration.** The moment the dashboard connects, it
  sends a timestamped ping over the link and the Arduino echoes it back
  immediately. The measured round-trip time is used to correct every
  reaction-time reading for that session, so the link's own transmission
  delay is never counted as part of the user's reflex.

```
Ultrasonic Sensors → Arduino Uno → Serial Link → Dashboard
                                        (game logic runs entirely here)
```

## A note on the serial link

The firmware sends identical data over **two transports at once**: a
direct USB serial connection, and a wired HC-05 Bluetooth module. Both
show up as options in the browser's own port picker when you click
Connect, and the latency calibration correctly measures whichever one you
choose — a real Bluetooth link has meaningfully more delay than USB, and
the calibration accounts for that either way.

Worth being upfront about: the Bluetooth path was the original plan, but
the specific HC-05 clone module used for this build turned out to be
unreliable close to the competition deadline, so the version actually
demonstrated ran over direct USB serial instead. The Bluetooth firmware
and protocol handling are fully implemented and should work with a
better-behaved module — see [`VERBOSE_SERIAL_DEBUG`](#troubleshooting)
below if you're chasing a similar issue.

## Hardware

| Component | Notes |
|---|---|
| Arduino Uno | Central controller |
| Ultrasonic sensor (HC-SR04) × 4 | Mounted in a row, 15 cm apart |
| HC-05 Bluetooth module | Optional — USB works without it |
| Jumper wires (male-to-female) | See wiring below |
| 9V battery + Uno barrel connector | Power |

No breadboard is required — see the wiring notes below for how the shared
sensor lines are joined without one.

## Wiring

| Connect | To |
|---|---|
| All 4 sensors' **TRIG** (tied together) | Arduino **D2** |
| Zone 1 sensor **ECHO** | Arduino **D3** |
| Zone 2 sensor **ECHO** | Arduino **D4** |
| Zone 3 sensor **ECHO** | Arduino **D5** |
| Zone 4 sensor **ECHO** | Arduino **D6** |
| HC-05 **TXD** | Arduino **D10** |
| HC-05 **RXD** | Arduino **D11** |
| All 4 sensors' **VCC** (daisy-chained) | Arduino **5V** |
| All 4 sensors' **GND** (daisy-chained) | Arduino **GND** |
| HC-05 **VCC** / **GND** | Arduino **5V** / **GND** |

**No breadboard, no problem:** since a shared line (TRIG, 5V, GND) would
need more than one wire in a single Arduino pin header hole, twist the
bare male ends of several male-to-female jumpers together by hand, then
plug one more male-to-female wire's female end over that twisted bundle
back to the Arduino pin. Wrap the joint in tape. Three such junctions
cover TRIG, VCC, and GND — every other connection (each ECHO pin, the
HC-05 lines) is a normal one-wire connection.

**Physical order matters:** whichever sensor is wired to D3 should be your
physical **leftmost** sensor, then D4/D5/D6 moving right, so the on-screen
zone numbering matches the physical layout.

HC-05's RX line is not always 5 V-tolerant — check your specific module's
datasheet before wiring it directly; some breakout boards include their
own level-shifting, others don't.

## Setup

1. Open `firmware/ReactionTrainer/ReactionTrainer.ino` in the Arduino IDE
   and upload it to an Arduino Uno.
2. Open `dashboard/index.html` directly in **Chrome or Edge** (Web Serial
   API support is required — Firefox and Safari won't work).
3. Click **Connect** and select the Arduino's port from the picker —
   either its direct USB serial port, or the HC-05's paired Bluetooth COM
   port if you've paired it in your OS Bluetooth settings first.
4. Wait for calibration to finish (a handful of round-trip pings — a
   couple of seconds), then choose a difficulty and mode and hit
   **Start Session**.

The dashboard's diagnostics strip shows live raw distance readings from
all four zones at all times — useful for confirming wiring, and for
tuning the trigger threshold (`TRIGGER_THRESHOLD_CM` near the top of the
dashboard's script, default 8 cm) to match your actual sensor mounting.

## Troubleshooting

If a serial connection opens but calibration never completes ("Calibration
ping timed out"), the fastest way to see what's actually happening is over
USB, independent of Bluetooth entirely:

1. Set `VERBOSE_SERIAL_DEBUG` to `1` near the top of the `.ino` file and
   re-upload.
2. Open the Arduino IDE's Serial Monitor at 115200 baud.
3. Watch what arrives while attempting to connect over Bluetooth from the
   dashboard: silence means nothing is reaching the Arduino at all
   (check wiring); garbled bracketed byte values mean a baud-rate mismatch
   between the Arduino and the HC-05; clean readable `PING:` text means
   the Arduino side is working correctly and the issue is elsewhere in the
   link.

## License

MIT — see [`LICENSE`](LICENSE).

## Team

Built by Kaustubha S. Bhanekar, Shreesha Rao K, and Shourya A. Kotian —
SDM School, Mangalore — for Canara Vikas 2026.
