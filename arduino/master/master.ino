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

const byte I2C_RX_LED_PIN = 10;
const byte I2C_TX_LED_PIN = 9;

void setup() {
  nextAddress = EEPROM.read(0);
  nextAddress = nextAddress == 255 ? 0 : nextAddress;
  
  Serial.begin(115200);
  Wire.begin(1); // join i2c bus (address optional for master)
  Wire.onRequest(sendAddress);
  Wire.onReceive(handleControlChange);

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

void handleControlChange() {
  toggleRxLed();
  Serial.println("Received event:");
  byte address = Wire.read();
  byte control = Wire.read();
  byte type = Wire.read();
  byte value = Wire.read();

  Serial.print("Address: ");
  Serial.print(address);
  Serial.print(", Control: ");
  Serial.print(control);
  Serial.print(", Type: ");
  Serial.print(type);
  Serial.print(", Value: ");
  Serial.println(value);
}
