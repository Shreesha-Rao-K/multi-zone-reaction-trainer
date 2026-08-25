// Multi-Zone Reaction Trainer — Arduino firmware
//
// Reads four ultrasonic sensors (mounted in a fixed horizontal row) and
// streams live distance readings to a laptop, over either the direct USB
// serial link or a wired Bluetooth (HC-05) module — both carry identical
// data, so either can be selected from the dashboard's port picker.
// All game logic (round selection, scoring, difficulty) lives in the
// browser dashboard; this sketch only senses and communicates.
//
// See ../../README.md for wiring, setup, and how the system works.

#include <SoftwareSerial.h>
#include <string.h>

// Set to 1 to print every raw byte received over Bluetooth to the USB
// Serial Monitor (as "[char:decimal]" per byte). Useful when diagnosing a
// Bluetooth link that isn't responding; noisy for normal use, so it
// defaults to off.
#define VERBOSE_SERIAL_DEBUG 0

const byte sharedTrigPin = 2;
const byte zoneEchoPins[] = {3, 4, 5, 6};
const byte zoneCount = 4;
const unsigned long echoTimeoutMicros = 25000UL;
const unsigned long triggerSettleMillis = 60UL;

SoftwareSerial bluetoothSerial(10, 11);

int lastZoneDistance[zoneCount] = {400, 400, 400, 400};
char incomingLine[32];
byte incomingLength = 0;
char usbIncomingLine[32];
byte usbIncomingLength = 0;

void setup() {
  pinMode(sharedTrigPin, OUTPUT);
  digitalWrite(sharedTrigPin, LOW);

  for (byte zone = 0; zone < zoneCount; zone++) {
    pinMode(zoneEchoPins[zone], INPUT);
  }

  bluetoothSerial.begin(9600);
  Serial.begin(115200);
  Serial.println("Reaction Trainer booting. Bluetooth link at 9600 baud on pins 10/11.");
}

void loop() {
  for (byte zone = 0; zone < zoneCount; zone++) {
    checkBluetoothInput();
    checkUsbInput();
    lastZoneDistance[zone] = readZoneDistance(zone);
    checkBluetoothInput();
    checkUsbInput();
    delay(triggerSettleMillis);
  }

  sendDistanceLine();
}

int readZoneDistance(byte zone) {
  // All modules receive this trigger; this pass records the selected zone's echo.
  digitalWrite(sharedTrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(sharedTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(sharedTrigPin, LOW);

  unsigned long pulseDuration = pulseIn(zoneEchoPins[zone], HIGH, echoTimeoutMicros);

  if (pulseDuration == 0) {
    return 400;
  }

  int distanceCm = pulseDuration / 58;

  if (distanceCm < 2 || distanceCm > 400) {
    return 400;
  }

  return distanceCm;
}

void sendDistanceLine() {
  bluetoothSerial.print("DIST:");

  for (byte zone = 0; zone < zoneCount; zone++) {
    bluetoothSerial.print(lastZoneDistance[zone]);

    if (zone < zoneCount - 1) {
      bluetoothSerial.print(',');
    }
  }

  bluetoothSerial.println();

  // Keep the USB protocol lines clean if Bluetooth debug bytes arrived first.
  Serial.println();
  Serial.print("DIST:");

  for (byte zone = 0; zone < zoneCount; zone++) {
    Serial.print(lastZoneDistance[zone]);

    if (zone < zoneCount - 1) {
      Serial.print(',');
    }
  }

  Serial.println();
}

void checkBluetoothInput() {
  while (bluetoothSerial.available() > 0) {
    char receivedCharacter = bluetoothSerial.read();

#if VERBOSE_SERIAL_DEBUG
    Serial.print('[');

    if (receivedCharacter >= 32 && receivedCharacter <= 126) {
      Serial.print(receivedCharacter);
    } else {
      Serial.print('.');
    }

    Serial.print(':');
    Serial.print((byte)receivedCharacter);
    Serial.print(']');
#endif

    if (receivedCharacter == '\r') {
      continue;
    }

    if (receivedCharacter == '\n') {
      incomingLine[incomingLength] = '\0';
      handleIncomingLine(incomingLine);
      incomingLength = 0;
      continue;
    }

    if (incomingLength < sizeof(incomingLine) - 1) {
      incomingLine[incomingLength] = receivedCharacter;
      incomingLength++;
    } else {
      incomingLength = 0;
    }
  }
}

void checkUsbInput() {
  while (Serial.available() > 0) {
    char receivedCharacter = Serial.read();

    if (receivedCharacter == '\r') {
      continue;
    }

    if (receivedCharacter == '\n') {
      usbIncomingLine[usbIncomingLength] = '\0';
      handleUsbIncomingLine(usbIncomingLine);
      usbIncomingLength = 0;
      continue;
    }

    if (usbIncomingLength < sizeof(usbIncomingLine) - 1) {
      usbIncomingLine[usbIncomingLength] = receivedCharacter;
      usbIncomingLength++;
    } else {
      usbIncomingLength = 0;
    }
  }
}

void handleIncomingLine(const char *line) {
  if (strncmp(line, "PING:", 5) == 0 && line[5] != '\0') {
    Serial.print("Received PING token=");
    Serial.print(line + 5);
    Serial.println(", replying PONG");
    bluetoothSerial.print("PONG:");
    bluetoothSerial.println(line + 5);
  }
}

void handleUsbIncomingLine(const char *line) {
  if (strncmp(line, "PING:", 5) == 0 && line[5] != '\0') {
    Serial.println();
    Serial.print("PONG:");
    Serial.println(line + 5);
  }
}
