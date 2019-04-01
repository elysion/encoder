#include <RotaryEncoder.h>

#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>

volatile int address;

enum controlTypes {
  DEBUG,
  ENCODER,
  BUTTON,
  SLIDER
};

#define USART_ENABLED 1

const byte ENC1A_INT = PCINT16;
const byte ENC1B_INT = PCINT11;
const byte ENCL1A_INT = PCINT19;
const byte ENCL1B_INT = PCINT20;
const byte ENCL2A_INT = PCINT18;
const byte ENCL2B_INT = PCINT17;
const byte ENCR1A_INT = PCINT6;
const byte ENCR1B_INT = PCINT7;
const byte ENCR2A_INT = PCINT0;
const byte ENCR2B_INT = PCINT1;

//NOTE: Current implementation expects the switches to be on the same port
const byte SW1_INT = PCINT8;
const byte SWL_INT = PCINT9;
const byte SWR_INT = PCINT10;
const byte SW_INTS_MASK = (1 << SW1_INT) | (1 << SWL_INT) | (1 << SWR_INT);
const byte SW_INTS[] = {PCINT8, PCINT9, PCINT10};

byte swStates = 0b111;
byte previousSwStates = swStates;

RotaryEncoder encoders[] = {
  RotaryEncoder(ENC1A, ENC1B), 
  RotaryEncoder(ENCL1A, ENCL1B),
#ifndef USART_ENABLED
  RotaryEncoder(ENCL2A, ENCL2B)
#endif
  RotaryEncoder(ENCR1A, ENCR1B), 
  RotaryEncoder(ENCR2A, ENCR2B)
};

int positions[] = {
  0, 
  0,
#ifndef USART_ENABLED
  0,
#endif 
  0, 
  0
};

byte pins[] = {
  ENC1A, ENC1B, 
  ENCL1A, ENCL1B,
#ifndef USART_ENABLED
  ENCL2A, ENCL2B,
#endif
  ENCR1A, ENCR1B, 
  ENCR2A, ENCR2B
};

byte states[] = {
  LOW, LOW, 
  LOW, LOW,
#ifndef USART_ENABLED
  LOW, LOW,
#endif
  LOW, LOW, 
  LOW, LOW
};

byte interrupter = 255;

void setup() {
  address = EEPROM.read(0);

  Serial.begin(115200);
  Serial.println("Boot");
  Serial.print("Address: ");
  Serial.println(address);
  
 if (address == 255 || address < 10) {
    Serial.print("Address: ");
    Serial.println(address);
    Serial.println("Requesting address from master");
    Wire.begin();
    Wire.requestFrom(1, 1);
    while (Wire.available()) {
      address = Wire.read();
      Serial.print("Received address: ");
      Serial.println(address);
    }
  
    Serial.print("Starting with address: ");
    Serial.println(address);
    Wire.begin(address);

    EEPROM.write(0, address);
  
    delay(900);
    handleStateChange(7, 1);
  } else {
    Wire.begin(address);
    handleStateChange(6, 1);
  }

  #ifndef USART_ENABLED
  Serial.end();
  #endif
  
  pinMode(ENC1A, INPUT_PULLUP);
  pinMode(ENC1B, INPUT_PULLUP);
  pinMode(ENCL1A, INPUT_PULLUP);
  pinMode(ENCL1B, INPUT_PULLUP);
  
#ifndef USART_ENABLED
  pinMode(ENCL2A, INPUT_PULLUP);
  pinMode(ENCL2B, INPUT_PULLUP);
#endif

  pinMode(ENCR1A, INPUT_PULLUP);
  pinMode(ENCR1B, INPUT_PULLUP);
  pinMode(ENCR2A, INPUT_PULLUP);
  pinMode(ENCR2B, INPUT_PULLUP);

  pinMode(SW1, INPUT_PULLUP);
  pinMode(SWR, INPUT_PULLUP);
  pinMode(SWL, INPUT_PULLUP);

  PCICR |= (1 << PCIE0) | (1 << PCIE1) | (1 << PCIE2);
  PCMSK2 |= (1 << ENC1A_INT) | (1 << ENCL1A_INT) | (1 << ENCL1B_INT);
  #ifndef USART_ENABLED 
  PCMSK2 |= (1 << ENCL2A_INT) | (1 << ENCL2B_INT);
  #endif
  PCMSK1 |= (1 << ENC1B_INT) | SW_INTS_MASK;
  PCMSK0 |= (1 << ENCR1A_INT) | (1 << ENCR1B_INT) | (1 << ENCR2A_INT) | (1 << ENCR2B_INT);
}

void loop() {
  if (previousSwStates != swStates) {
    #ifdef USART_ENABLED
    Serial.print("SWITCHES: ");
    Serial.println(swStates);
    #endif
    byte changed = previousSwStates ^ swStates;
    previousSwStates = swStates;
    #ifdef USART_ENABLED
    Serial.print("Changed: ");
    Serial.println(changed);
    #endif
    if (changed) {
      for (byte i = 0; i < 3; ++i) {
        byte swMask = (1 << SW_INTS[i]);
        if (changed & swMask) {
          handleButtonChange(i, (swStates & swMask) ? 0 : 1);
        }
      }
    }
  }
  
  for (int i = 0; i < 4; ++i) {
    int position = encoders[i].getPosition();
    if (position != positions[i]) {
      handleStateChange(i, position);
      positions[i] = position;
    }
    
    #ifdef USART_ENABLED
    byte stateA = digitalRead(pins[2*i]);
    byte stateB = digitalRead(pins[2*i+1]);

    if (stateA != states[2*i] || stateB != states[2*i+1]) {
      Serial.print("Interrupter: ");
      Serial.println(interrupter);
      Serial.print("Pins ");
      Serial.println(i);
      Serial.println("A B");
      Serial.print(stateA);
      Serial.print(" ");
      Serial.println(stateB);
      states[2*i] = stateA;
      states[2*i+1] = stateB;
    }
    #endif
  }

  #ifdef USART_ENABLED 
  if (Serial.available()) {      // If anything comes in Serial (USB),
    Serial.write(Serial.read());   // read it and send it out Serial1 (pins 0 & 1)
  }
  #endif
}

void handleStateChange(byte input, byte state) {
  Wire.beginTransmission(1);
  byte message[] = {address, input, ENCODER, state};
  Wire.write(message, 4);
  Wire.endTransmission();
}

void handleButtonChange(byte input, byte state) {
  Wire.beginTransmission(1);
  byte message[] = {address, input, BUTTON, state};
  Wire.write(message, 4);
  Wire.endTransmission();
}

ISR(PCINT0_vect) {
  interrupter = 0;
  #ifdef USART_ENABLED 
  encoders[2].tick();
  #endif
  encoders[3].tick();
  #ifndef USART_ENABLED
  encoders[4].tick();
  #endif
}

ISR(PCINT1_vect) {
  interrupter = 1;
  encoders[0].tick();

  swStates = PINC & SW_INTS_MASK;
}

ISR(PCINT2_vect) {
  interrupter = 2;
  encoders[0].tick();
  encoders[1].tick();
  #ifndef USART_ENABLED 
  encoders[2].tick();
  #endif
}
