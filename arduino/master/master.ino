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
}

void loop() {
  delay(100);
  Serial.print(".");
}

void sendAddress() {
  Wire.write(nextAddress);
  nextAddress++;
  EEPROM.write(0, nextAddress);
  Serial.println("Sent address:");
  Serial.println(nextAddress);
}

void handleControlChange() {
  Serial.println("Received event:");
  byte address = Wire.read();
  byte control = Wire.read();
  byte type = Wire.read();
  byte value = Wire.read();

  Serial.println(address);
  Serial.println(control);
  Serial.println(type);
  Serial.println(value);
}
