#include <RotaryEncoder.h>

#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>

#include "shared.h"
#include "slave.h"

volatile int address;

#define USART_DEBUG_ENABLED 1

byte switchStates;
byte previousSwitchStates;
byte touchStates;
byte previousTouchStates;

const byte BOARD_FEATURES[5] = {
  NO_BOARD
  #ifdef USART_DEBUG_ENABLED
    & !BOARD_FEATURE_ENCODER & !BOARD_FEATURE_TOUCH // L2 encoder pins are connected to main boards RX & TX and hence cannot be used when using USART
  #endif
  ,
  NO_BOARD, 
  BOARD_FEATURE_POT, 
  NO_BOARD,
  NO_BOARD
};

RotaryEncoder encoders[5] = { // TODO: Allocate memory dynamically?
  RotaryEncoder(ENCL2A, ENCL2B),
  RotaryEncoder(ENCL1A, ENCL1B),
  RotaryEncoder(ENC1A, ENC1B), 
  RotaryEncoder(ENCR1A, ENCR1B), 
  RotaryEncoder(ENCR2A, ENCR2B)
};

int positions[] = {
  0, 
  0,
  0,
  0, 
  0
};

#ifdef USART_DEBUG_ENABLED
byte states[] = {
  LOW, LOW, 
  LOW, LOW,
  LOW, LOW,
  LOW, LOW, 
  LOW, LOW
};
#endif

byte interrupter = 255;

void(* reset) (void) = 0;

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

    if (address == 255) {
      Serial.println("Did not receive address from master. Resetting.");
      reset();
    }
  
    Serial.print("Starting with address: ");
    Serial.println(address);
    Wire.begin(address);

    EEPROM.write(0, address);
  
    delay(900);
    sendMessage(DEBUG_RECEIVED_ADDRESS, address, CONTROL_TYPE_DEBUG);
  } else {
    Wire.begin(address);
    sendMessage(DEBUG_BOOT, 1, CONTROL_TYPE_DEBUG);
  }

  #ifndef USART_DEBUG_ENABLED
  Serial.end();
  #endif

  for (byte i = 0; i < 5; ++i) {
    if (BOARD_FEATURES[i] & BOARD_FEATURE_ENCODER) {
      pinMode(ENCODER_PINS[i*2], INPUT_PULLUP);
      pinMode(ENCODER_PINS[i*2 + 1], INPUT_PULLUP);
    }

    if (BOARD_FEATURES[i] & BOARD_FEATURE_BUTTON) {
      pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    }

    if (BOARD_FEATURES[i] & BOARD_FEATURE_POT) {
      // TODO: anything needed here?
    }
    
    if (BOARD_FEATURES[i] & BOARD_FEATURE_TOUCH) {
      pinMode(TOUCH_PINS[i], INPUT);
    }
  }

  // TODO: Move interupt initializations to loop above
  PCICR |= (1 << PCIE0) | (1 << PCIE1) | (1 << PCIE2);
  
  #ifndef USART_DEBUG_ENABLED
  if (BOARD_FEATURES[BOARD_L2] & BOARD_FEATURE_ENCODER) {
    PCMSK2 |= (1 << ENCL2A_INT) | (1 << ENCL2B_INT);
  }
  #endif

  if (BOARD_FEATURES[BOARD_L1] & BOARD_FEATURE_ENCODER) {
    PCMSK2 |= (1 << ENCL1A_INT) | (1 << ENCL1B_INT);
  }

  if (BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_ENCODER) {
    PCMSK1 |= (1 << ENC1B_INT);
    PCMSK2 |= (1 << ENC1A_INT);
  }
  
  if (BOARD_FEATURES[BOARD_R1] & BOARD_FEATURE_ENCODER) {
    PCMSK0 |= (1 << ENCL1A_INT) | (1 << ENCL1B_INT);
  }

  if (BOARD_FEATURES[BOARD_R1] & BOARD_FEATURE_ENCODER) {
    PCMSK0 |= (1 << ENCR2A_INT) | (1 << ENCR2B_INT);
  }

  if (BOARD_FEATURES[BOARD_L1] & (BOARD_FEATURE_BUTTON | BOARD_FEATURE_TOUCH)) {
    PCMSK1 |= 1 << SWL_INT;
  }
    
  if (BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_BUTTON) {
    PCMSK1 |= 1 << SW1_INT;
  }

  if (BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_TOUCH) {
    PCMSK1 |= 1 << TOUCH_INT;
  }
    
  if (BOARD_FEATURES[BOARD_R1] & (BOARD_FEATURE_BUTTON | BOARD_FEATURE_TOUCH)) {
    PCMSK1 |= 1 << SWR_INT;
  }

  // TODO: initialize according to enabled buttons
  switchStates = previousSwitchStates = PINC & SW_INTS_MASK; // TODO: construct mask according to enabled buttons
  // TODO: initialize touch states
}

void loop() {
  // TODO: check touch
  if (previousSwitchStates != switchStates) {
    #ifdef USART_DEBUG_ENABLED
    Serial.print("SWITCHES: ");
    Serial.println(switchStates);
    #endif
    byte changed = previousSwitchStates ^ switchStates;
    previousSwitchStates = switchStates;
    #ifdef USART_DEBUG_ENABLED
    Serial.print("Changed: ");
    Serial.println(changed);
    #endif
    if (changed) {
      for (byte i = 0; i < 3; ++i) {
        byte switchMask = (1 << SW_INTS[i]);
        if (changed & switchMask) {
          handleButtonChange(i, (switchStates & switchMask) ? 0 : 1);
        }
      }
    }
  }
  
  for (int i = 0; i < 5; ++i) {
    int position;
    byte positionChanged = false;
    
    if (BOARD_FEATURES[i] & BOARD_FEATURE_ENCODER) {
      position = encoders[i].getPosition(); // TODO: use getDirection or create separate relative encoder feature
      positionChanged = position != positions[i];
      
      #ifdef USART_DEBUG_ENABLED
      byte stateA = digitalRead(ENCODER_PINS[2*i]);
      byte stateB = digitalRead(ENCODER_PINS[2*i+1]);
  
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
    } else if (BOARD_FEATURES[i] & BOARD_FEATURE_POT) {
      positionChanged = position != positions[i] && (position == 0 || position == 127 || POT_CHANGE_THRESHOLD < abs(positions[i] - position));
      // Resolution restricted to 7-bits for MIDI compatibility
      position = analogRead(POT_PINS[i]) >> 3;
    } else {
      continue;
    }
    
    if (positionChanged) {
      handlePositionChange(i, position);
      positions[i] = position;
    }
  }

  #ifdef USART_DEBUG_ENABLED 
  if (Serial.available()) {        // If anything comes in Serial,
    Serial.write(Serial.read());   // read it and send it out
  }
  #endif
}

void handlePositionChange(byte input, byte state) { // state cannot be byte if full resolution of ADC is used (max value = 1023)
  sendMessage(input, state, CONTROL_TYPE_POSITION);
}

void handleButtonChange(byte input, byte state) {
  sendMessage(input, state, CONTROL_TYPE_BUTTON);
}

void sendMessage(byte input, byte value, ControlType type) {
  Wire.beginTransmission(1);
  byte message[] = {address, input, type, value};
  Wire.write(message, 4);
  Wire.endTransmission();
}

ISR(PCINT0_vect) {
  interrupter = 0;
  encoders[BOARD_R1].tick();
  encoders[BOARD_R2].tick();
}

ISR(PCINT1_vect) {
  interrupter = 1;
  encoders[BOARD_M].tick();

  // TODO: test if boards have buttons
  switchStates = PINC & SW_INTS_MASK;
}

ISR(PCINT2_vect) {
  interrupter = 2;
  // TODO: is it ok to tick even if not using encoders?
  encoders[BOARD_M].tick();
  encoders[BOARD_L1].tick();
  #ifndef USART_DEBUG_ENABLED 
  encoders[BOARD_L2].tick();
  #endif

  touchStates = digitalRead(TOUCH) << BOARD_M | digitalRead(SWL) << BOARD_L1 | digitalRead(SWR) << BOARD_R1; // TODO: only read pin values if touch features are enabled
}
