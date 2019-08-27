#include <MIDIUSB_Defs.h>
#include <MIDIUSB.h>
#include <Wire.h>
#include <EEPROM.h>

#include "shared.h"

//#include <stdarg.h>
//void p(char *fmt, ... ){
//        char buf[128]; // resulting string limited to 128 chars
//        va_list args;
//        va_start (args, fmt );
//        vsnprintf(buf, 128, fmt, args);
//        va_end (args);
//        Serial.print(buf);
//}

volatile byte nextAddress;

const uint8_t SS1Pin = 4;

char* DEBUG_CODES[] = {
  "BOOT",
  "RECEIVED_ADDRESS",
  "BYTE",
  "STATE",
  "INTERRUPTER",
  "POSITION",
  "CHANGE",
  "TICKS",
  "PORTB",
  "PORTC",
  "PORTD"
};

char* CONTROL_TYPE_NAMES[] = {
  "DEBUG",
  "ENCODER",
  "BUTTON",
  "POSITION",
  "TOUCH",
  "DATA"
};

void setup() {
  nextAddress = EEPROM.read(0);
  nextAddress = nextAddress == 255 ? 0 : nextAddress;
  
  Serial.begin(115200);
  Serial.println("Boot");
  Wire.begin(1); // join i2c bus (address optional for master)
  Wire.onRequest(sendAddress);
  Wire.onReceive(handleControlChange);

  pinMode(SS1Pin, OUTPUT);
  digitalWrite(SS1Pin, LOW);
  Serial.print("Next address: ");
  Serial.println(nextAddress);
}

void loop() {
  delay(100);
  Serial.print(".");
  MidiUSB.flush();
}

void sendAddress() {
  Wire.write(nextAddress);
  nextAddress++;
  EEPROM.write(0, nextAddress);
  Serial.println("Sent address:");
  Serial.println(nextAddress);
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void handleControlChange() {
  byte address = Wire.read();
  byte control = Wire.read();
  byte type = Wire.read();
  byte value = Wire.read();

  Serial.println();
  byte channel = address - 171; // TODO: reset 171 to 0
  if (type == CONTROL_TYPE_DEBUG) {
    Serial.print("Address: ");
    Serial.print(address);
    Serial.print(", Control: ");
    Serial.print(control);
    Serial.print(", Debug: ");
    if (control <= 10) {
      Serial.print(DEBUG_CODES[control]);
    } else {
      Serial.print("out of bounds!");
    }
    Serial.print(", Value: ");
    Serial.println(value);
  } else {
    Serial.print("Address: ");
    Serial.print(address);
    Serial.print(", Control: ");
    Serial.print(control);
    Serial.print(", Type: ");
    Serial.print(CONTROL_TYPE_NAMES[type]);
    Serial.print(", Value: ");
    Serial.println(value);
    // {address, input, type, value}
    if (type == CONTROL_TYPE_BUTTON) {
      if (value == 0) {
        noteOn(channel, control, 127);
      } else {
        noteOff(channel, control, 0);
      }
    } else {
      controlChange(channel, control, value == 1 ? 1 : 127);
    }
  }
}
