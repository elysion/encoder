#include <MIDIUSB.h>
#include <Wire.h>
#include <EEPROM.h>
#include <limits.h>

#include "shared.h"

volatile byte nextAddress;
volatile byte nextAddressIndex = 0;

const byte CHANNEL_COUNT = 15;
const byte ADDRESS_NOT_FOUND = CHAR_MAX;
volatile byte addressToMidiChannels[CHANNEL_COUNT];

const uint8_t SS1Pin = 4;

const byte I2C_RX_LED_PIN = 10;
const byte I2C_TX_LED_PIN = 9;

void setup() {
  nextAddress = EEPROM.read(0);
  nextAddress = nextAddress == 255 ? 0 : nextAddress;

  Serial.begin(115200);
  Wire.begin(MASTER_ADDRESS); // join i2c bus (address optional for master)

  for (byte i = 0; i < CHANNEL_COUNT; ++i) {
    byte address = EEPROM.read(i+1);
    if (address == 255) break;

    addressToMidiChannels[i] = address;
    nextAddressIndex++;
  }

  Wire.onRequest(sendAddress);
  Wire.onReceive(handleControlChange);

  Serial.begin(115200);

  Serial.println("Boot");
  printChannels();

  pinMode(SS1Pin, OUTPUT);
  digitalWrite(SS1Pin, LOW);
  Serial.print("Next address: ");
  Serial.println(nextAddress);

  pinMode(I2C_RX_LED_PIN, OUTPUT);
  digitalWrite(I2C_RX_LED_PIN, HIGH);

  pinMode(I2C_TX_LED_PIN, OUTPUT);
  digitalWrite(I2C_TX_LED_PIN, HIGH);
}

void loop() {
  delay(100);
  Serial.print(".");
}

void printChannels() {
  for (byte i = 0; i < CHANNEL_COUNT; ++i) {
    Serial.print("Restored ");
    Serial.print(addressToMidiChannels[i]);
    Serial.print(" to index ");
    Serial.println(i);
  }
}

inline void togglePin(byte outputPin) {
  digitalWrite(outputPin, !digitalRead(outputPin));
}

inline void toggleRxLed() {
  togglePin(I2C_RX_LED_PIN);
}

inline void toggleTxLed() {
  togglePin(I2C_TX_LED_PIN);
}

void sendAddress() {
  toggleTxLed();
  Wire.write(nextAddress);
  nextAddress++;
  EEPROM.write(0, nextAddress);
  Serial.println("Sent address:");
  Serial.println(nextAddress);
}

SlaveToMasterMessage readMessage() {
  byte address = Wire.read();
  byte input = Wire.read();
  byte type = Wire.read();
  byte valueHighByte = Wire.read();
  byte valueLowByte = Wire.read();

  SlaveToMasterMessage message = {address, input, type, word(valueHighByte, valueLowByte)};
  return message;
}

byte findChannelForAddress(byte address) {
  for (byte i = 0; i < CHANNEL_COUNT; ++i) {
    if (addressToMidiChannels[i] == address) return i;
  }

  return ADDRESS_NOT_FOUND;
}

void saveAddressAsNextChannel(byte address) {
  Serial.print("Storing ");
  Serial.print(address);
  Serial.print(" to index ");
  Serial.println(nextAddressIndex);

  addressToMidiChannels[nextAddressIndex] = address;
  EEPROM.write(nextAddressIndex+1, address);
  nextAddressIndex++;
}

void handleControlChange() {
  toggleRxLed();
  Serial.println("Received event:");

  SlaveToMasterMessage message = readMessage();
  const uint16_t value = message.value;
  const ControlType type = message.type;
  const uint8_t input = message.input;
  const uint8_t address = message.address;

  Serial.print("Address: ");
  Serial.print(address);
  Serial.print(", Control: ");
  Serial.print(input);
  Serial.print(", Type: ");
  Serial.print(type);
  Serial.print(", Value: ");
  Serial.println(value);

  if (type == CONTROL_TYPE_DEBUG && value == DEBUG_BOOT) {
    saveAddressAsNextChannel(address); // TODO: slaves do not send their address at boot :facepalm:
  }
  if (type == CONTROL_TYPE_POSITION) {
    byte channel = findChannelForAddress(address);
    if (channel == ADDRESS_NOT_FOUND) {
      channel = nextAddressIndex;
      saveAddressAsNextChannel(address);
    }
    controlChange(channel, control, value == 1 ? 1 : 127);
  }
  if (type == CONTROL_TYPE_BUTTON) {
    byte channel = findChannelForAddress(address);
    if (channel == ADDRESS_NOT_FOUND) {
      channel = nextAddressIndex;
      saveAddressAsNextChannel(address);
    }

    if (value == 1) {
      noteOn(channel, control, 127);
    } else {
      noteOff(channel, control, 0);
    }
  }
}

void controlChange(byte channel, byte control, byte value) {
  Serial.print("Sending CC: channel: ");
  Serial.print(channel);
  Serial.print(", control: ");
  Serial.print(control);
  Serial.print(", value: ");
  Serial.println(value);

  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
}

void noteOn(byte channel, byte pitch, byte velocity) {
  Serial.print("Sending NoteOn: channel: ");
  Serial.print(channel);
  Serial.print(", pitch: ");
  Serial.print(pitch);
  Serial.print(", velocity: ");
  Serial.println(velocity);

  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void noteOff(byte channel, byte pitch, byte velocity) {
  Serial.print("Sending NoteOff: channel: ");
  Serial.print(channel);
  Serial.print(", pitch: ");
  Serial.print(pitch);
  Serial.print(", velocity: ");
  Serial.println(velocity);

  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}
