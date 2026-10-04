# Multi-Zone Reaction Trainer

A reflex-training device that tests more than just how fast you react — it
tests whether you react in the *right place*. Four ultrasonic sensors,
mounted in a fixed row, each watch their own zone; a laptop dashboard picks
one at random each round, times your response, and automatically corrects
for the communication link's own delay so that delay is never mistaken for
part of your reflex.

Built for **VIKRAMA 2026** — a Science Modelling Competition organized by
Akhil Bharatiya Vidyarthi Parishad (ABVP), Mangaluru, in association with
Canara Educational Institutions, held at Canara Vikaas PU College,
Mangaluru, to commemorate the birth anniversary of Vikram Sarabhai.

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
  delay is never counted as part of the user's reflex. For full test
  methodology and empirical 100-cycle jitter data, see the
  [Latency Calibration Benchmark Report](docs/benchmark-2026-09.md).

```
Ultrasonic Sensors → Arduino Uno → Serial Link → Dashboard
                                        (game logic runs entirely here)
```

## Serial Transports & Baud Rates

The firmware supports two communication interfaces:

- **Primary Verified Configuration — Direct USB Serial @ 115,200 baud**: The official configuration used during the VIKRAMA 2026 live competition demonstration and documented in the latency benchmarks. High throughput with minimal packet serialization delay (`Serial.begin(115200)`).
- **Wireless Bluetooth — HC-05 Module @ 9,600 baud**: Operates via Arduino `SoftwareSerial` on pins 10 (TX) and 11 (RX) at 9,600 baud (`bluetoothSerial.begin(9600)`).

In the web dashboard, use the **Transport toggle** above the connect button:
- `USB (115200)` opens the Web Serial port at 115,200 baud.
- `HC-05 BT (9600)` opens the paired Bluetooth COM port at 9,600 baud.

Once connected, the 8-sample latency calibration handshake automatically measures round-trip time and estimates one-way link transit delay for whichever transport is selected.

> **Engineering context:** The Bluetooth path was part of the original design, but the specific HC-05 clone module proved erratic close to the competition deadline. The build actually demonstrated ran over direct USB serial. Both paths are fully implemented in firmware and supported by the dashboard transport selector.

## Acoustic Isolation & Trigger Architecture

- **Competition Build (Shared TRIG on D2)**: In the original VIKRAMA 2026 build, all four sensor TRIG pins were joined to Arduino pin D2 without a breadboard to reduce wire harness weight and bulk. To prevent acoustic reflections from overlapping across zones, the firmware enforces a 60 ms inter-zone settling pause (`triggerSettleMillis = 60UL`) between zone measurements. At standard sound speed (~343 m/s), 60 ms allows sonic waves to travel >20 meters and dissipate before the subsequent sensor is triggered.
- **Independent Trigger Configuration (`INDEPENDENT_TRIG_PINS 1`)**: For environments with heavy reflective acoustic boundaries or tight enclosures, set `#define INDEPENDENT_TRIG_PINS 1` in `ReactionTrainer.ino` and wire Zone 1–4 TRIG lines to pins `D2, D7, D8, D9`. This completely isolates ultrasonic bursts to only the active zone being polled.

## Hardware

| Component | Notes |
|---|---|
| Arduino Uno | Central controller |
| Ultrasonic sensor (HC-SR04) × 4 | Mounted in a row, 15 cm apart |
| HC-05 Bluetooth module | Optional — USB works without it |
| Jumper wires (male-to-female) | See wiring below |
| 9V battery + Uno barrel connector | Power |

No breadboard is required for the competition build — see the wiring notes below for how the shared sensor lines are joined.

## Wiring (Competition Build)

| Connect | To |
|---|---|
| All 4 sensors' **TRIG** (tied together) | Arduino **D2** *(or D2, D7, D8, D9 if INDEPENDENT_TRIG_PINS enabled)* |
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

Built by Shreesha Rao K —
SDM School, Mangalore — for VIKRAMA 2026.
