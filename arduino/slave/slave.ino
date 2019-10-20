#include <MillisTimer.h>
#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Wire.h>
#include <EEPROM.h>

#define USART_DEBUG_ENABLED 1
#define SKIP_FEATURE_VALIDATION

#include "config.h"
#include "shared.h"
#include "slave.h"

const uint16_t PixelCount = 8;
#if PCB_VERSION == 3
const uint8_t PixelPin = LEDL;
#else
const uint8_t PixelPin = LED1;
#endif

Adafruit_NeoPixel pixels(PixelCount, PixelPin, NEO_GRB + NEO_KHZ800);

volatile int address;

byte switchStates;
byte previousSwitchStates;

#if PCB_VERSION != 3 // TODO
byte touchStates;
byte previousTouchStates;
#endif

byte padStates[] =  {
  0b00001111,
  0b00001111,
  0b00001111
};
byte previousPadStates[] = {
  0b00001111,
  0b00001111,
  0b00001111
};

#include "feature_validation.h"

RotaryEncoder* encoders[BOARD_COUNT] = {
  #if BOARD_FEATURES_L2 & BOARD_FEATURE_ENCODER
  new RotaryEncoder(ENCODER_PINS[BOARD_L2][0], ENCODER_PINS[BOARD_L2][1])
  #else
  0
  #endif
  ,
  #if BOARD_FEATURES_L1 & BOARD_FEATURE_ENCODER
  new RotaryEncoder(ENCODER_PINS[BOARD_L1][0], ENCODER_PINS[BOARD_L1][1])
  #else
  0
  #endif
  ,
#if PCB_VERSION == 3
  #if BOARD_FEATURES_M1 & BOARD_FEATURE_ENCODER
  new RotaryEncoder(ENCODER_PINS[BOARD_M1][0], ENCODER_PINS[BOARD_M1][1])
  #else
  0
  #endif
  ,
  #if BOARD_FEATURES_M2 & BOARD_FEATURE_ENCODER
  new RotaryEncoder(ENCODER_PINS[BOARD_M2][0], ENCODER_PINS[BOARD_M2][1])
  #else
  0
  #endif
#else
  #if BOARD_FEATURES_M & BOARD_FEATURE_ENCODER
  new RotaryEncoder(ENCODER_PINS[BOARD_M][0], ENCODER_PINS[BOARD_M][1])
  #else
  0
  #endif
#endif
  
  ,
  #if BOARD_FEATURES_R1 & BOARD_FEATURE_ENCODER
  new RotaryEncoder(ENCODER_PINS[BOARD_R1][0], ENCODER_PINS[BOARD_R1][1])
  #else
  0
  #endif
  ,
  #if BOARD_FEATURES_R2 & BOARD_FEATURE_ENCODER
  new RotaryEncoder(ENCODER_PINS[BOARD_R2][0], ENCODER_PINS[BOARD_R2][1])
  #else
  0
  #endif
};

int positions[] = {
  0, 
  0,
  0,
#if PCB_VERSION == 3
  0,
#endif
  0, 
  0
};

#if USART_DEBUG_ENABLED
byte states[] = {
  LOW, LOW, 
  LOW, LOW,
  LOW, LOW,
#if PCB_VERSION == 3
  LOW, LOW,
#endif
  LOW, LOW, 
  LOW, LOW
};
#endif

byte interrupter = 255;
#define LED_BUILTIN_AVAILABLE (BOARD_FEATURES_R2 == NO_BOARD) // TODO: only disable when R2 uses encoder? (How does the pull-up on the pin affect this need?) 

void blinkBuiltinLed(MillisTimer &timer __attribute__((unused))) {
  toggleBuiltinLed();
}
MillisTimer blinkTimer = MillisTimer(1000, blinkBuiltinLed);

void(* reset) (void) = 0;

inline void togglePin(byte outputPin) {
  digitalWrite(outputPin, !digitalRead(outputPin));
}

inline void toggleBuiltinLed() {
#if PCB_VERSION == 3 && LED_BUILTIN_AVAILABLE
    togglePin(LED_BUILTIN);
#endif
}

void setup() {
  if (LED_BUILTIN_AVAILABLE) {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
  }
  
  noInterrupts();
  pixels.begin();
  interrupts();

  Serial.begin(115200);
  
  setupI2c();

  #ifndef USART_DEBUG_ENABLED
  Serial.end();
  #endif

  setupPinModes();
  // TODO: Move interrupt initializations to the loop in setupPinModes();
  setupInterrupts();

  // TODO: initialize according to enabled buttons
  switchStates = previousSwitchStates = getButtonStates(); // TODO: construct mask according to enabled buttons
  // TODO: initialize touch states

  blinkTimer.start();
}

const int FIRST_BUTTON_VOLTAGE = 981;
const int SECOND_BUTTON_VOLTAGE = 660;
const int BOTH_BUTTONS_VOLTAGE = 828;
const int BUTTON_VOLTAGE_RANGE = min(BOTH_BUTTONS_VOLTAGE - SECOND_BUTTON_VOLTAGE, FIRST_BUTTON_VOLTAGE - BOTH_BUTTONS_VOLTAGE) / 2;

bool isInRange(int value, int target, int range) {
  return value > target - range && value < target + range;
}

ButtonPairStates voltageToButtonStates(int voltage) {
  ButtonPairStates buttonStates;
  bool bothButtonsPressed = isInRange(voltage, BOTH_BUTTONS_VOLTAGE, BUTTON_VOLTAGE_RANGE);
  buttonStates.firstButtonState = bothButtonsPressed || isInRange(voltage, FIRST_BUTTON_VOLTAGE, BUTTON_VOLTAGE_RANGE);
  buttonStates.secondButtonState = bothButtonsPressed || isInRange(voltage, SECOND_BUTTON_VOLTAGE, BUTTON_VOLTAGE_RANGE);
  return buttonStates;
}

byte getButtonStates() {
#if PCB_VERSION == 3
  byte buttonStates = 0;
#if HAS_FEATURE(L1, BOARD_FEATURE_BUTTON) || HAS_FEATURE(L2, BOARD_FEATURE_BUTTON)
  int swlVoltage = analogRead(SWL);
  ButtonPairStates lButtonStates = voltageToButtonStates(swlVoltage);
  buttonStates |= ((lButtonStates.firstButtonState ? 1 : 0) | ((lButtonStates.secondButtonState ? 1 : 0) << 1));
#endif
#if HAS_FEATURE(M1, BOARD_FEATURE_BUTTON) || HAS_FEATURE(M2, BOARD_FEATURE_BUTTON)
  int swmVoltage = analogRead(SWM);
  ButtonPairStates mButtonStates = voltageToButtonStates(swmVoltage);
  buttonStates |= (((mButtonStates.firstButtonState ? 1 : 0) << 2) | ((mButtonStates.secondButtonState ? 1 : 0) << 3)); // TODO: only take the buttons into account if they are enabled
#endif
#if HAS_FEATURE(R1, BOARD_FEATURE_BUTTON) || HAS_FEATURE(R2, BOARD_FEATURE_BUTTON)
  int swrVoltage = analogRead(SWR);
  ButtonPairStates rButtonStates = voltageToButtonStates(swrVoltage);
  buttonStates |= (((rButtonStates.firstButtonState ? 1 : 0) << 4) | ((rButtonStates.secondButtonState ? 1 : 0) << 5));
#endif
#else
  return PINC & SW_INTS_MASK;
#endif

  return buttonStates;
}

inline void setupInterrupts() {
  PCICR |= (1 << PCIE0) | (1 << PCIE1) | (1 << PCIE2);
  
  #ifndef USART_DEBUG_ENABLED
  if (BOARD_FEATURES[BOARD_L2] & BOARD_FEATURE_ENCODER) {
    enablePCINT(ENCL2A);
    enablePCINT(ENCL2B);
  }
  #endif

  if (BOARD_FEATURES[BOARD_L1] & BOARD_FEATURE_ENCODER) {
    enablePCINT(ENCL1A);
    enablePCINT(ENCL1B);
  }

#if PCB_VERSION == 3
  if (BOARD_FEATURES[BOARD_M1] & BOARD_FEATURE_ENCODER) {
    enablePCINT(ENCM1B);
    enablePCINT(ENCM1A);
  }
  if (BOARD_FEATURES[BOARD_M2] & BOARD_FEATURE_ENCODER) {
    enablePCINT(ENCM2B);
    enablePCINT(ENCM2A);
  }
#else
  if (BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_ENCODER) {
    enablePCINT(ENC1B);
    enablePCINT(ENC1A);
  }
#endif

  if (BOARD_FEATURES[BOARD_R1] & BOARD_FEATURE_ENCODER) {
    enablePCINT(ENCR1A);
    enablePCINT(ENCR1B);
  }
  
  if (BOARD_FEATURES[BOARD_R2] & BOARD_FEATURE_ENCODER) {
    enablePCINT(ENCR2A);
    enablePCINT(ENCR2B);
  }
  
#if PCB_VERSION != 3 // v3 does not have a PCINT on SWL :(
  if (BOARD_FEATURES[BOARD_L1] & (BOARD_FEATURE_BUTTON | BOARD_FEATURE_TOUCH)) {
    enablePCINT(SWL);
  }
#endif

#if PCB_VERSION == 3
  if (BOARD_FEATURES[BOARD_M1] & BOARD_FEATURE_BUTTON || BOARD_FEATURES[BOARD_M2] & BOARD_FEATURE_BUTTON) {
    enablePCINT(SWM);
  }
#else
  if (BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_BUTTON) {
    enablePCINT(SWM);
  }
  if (BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_TOUCH) {
    enablePCINT(TOUCH);
  }
#endif

#if PCB_VERSION != 3 // v3 does not have a PCINT on SWR :(
  if (BOARD_FEATURES[BOARD_R1] & (BOARD_FEATURE_BUTTON | BOARD_FEATURE_TOUCH)) {
    enablePCINT(SWR);
  }
#endif

#if PCB_VERSION != 3 // TODO
  if (BOARD_FEATURES[BOARD_L1] & BOARD_FEATURE_PADS) {
    enablePCINT(SWL);
    enablePCINT(ENCL1A);
    enablePCINT(ENCL1B);
    enablePCINT(ENCL2A);
  }

  if (BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_PADS) {
    enablePCINT(SWM);
    enablePCINT(ENC1B);
    enablePCINT(ENC1A);
    enablePCINT(TOUCH); // TODO: fix  POT -> TOUCH on board
  }
  
  if (BOARD_FEATURES[BOARD_R1] & BOARD_FEATURE_PADS) {
    enablePCINT(ENCR1A);
    enablePCINT(ENCR1B);
    enablePCINT(ENCR2A);
    enablePCINT(SWR);
  }
#endif
}

inline void setupPinModes() {
  for (byte i = 0; i < BOARD_COUNT; ++i) {
    const byte boardFeatures = BOARD_FEATURES[i];

    if (boardFeatures & BOARD_FEATURE_ENCODER) {
      pinMode(ENCODER_PINS[i][0], INPUT_PULLUP);
      pinMode(ENCODER_PINS[i][1], INPUT_PULLUP);
    }

    if (boardFeatures & BOARD_FEATURE_BUTTON) {
#if PCB_VERSION == 3
      pinMode(BUTTON_PINS[i], INPUT);
#else
      pinMode(BUTTON_PINS[i], INPUT_PULLUP);
#endif      
    }

    if (boardFeatures & BOARD_FEATURE_POT) {
      // TODO: anything needed here?
    }

#if PCB_VERSION != 3 // TODO
    if (boardFeatures & BOARD_FEATURE_TOUCH) {
      pinMode(TOUCH_PINS[i], INPUT);
    }
#endif

#if PCB_VERSION != 3 // TODO
    if (BOARD_FEATURES[i] & BOARD_FEATURE_PADS) {
      for (byte j = 0; j < 4; ++j) {
        #if USART_DEBUG_ENABLED
        Serial.print("Configuring pin as input: ");
        Serial.println(PAD_PINS[i][j]);
        #endif
        pinMode(PAD_PINS[i][j], INPUT_PULLUP);
      }
    } 
#endif
  }
}

inline void setupI2c() {
  address = EEPROM.read(0);
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
}

void loop() {
  blinkTimer.run();

  // TODO: check touch
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
delay(500);
  int swmVoltage = analogRead(SWM);
  sendMessage(1, abs(swmVoltage - FIRST_BUTTON_VOLTAGE) & 0xFF, abs(swmVoltage - FIRST_BUTTON_VOLTAGE) >> 8);
  sendMessage(2, abs(swmVoltage - SECOND_BUTTON_VOLTAGE) & 0xFF, abs(swmVoltage - SECOND_BUTTON_VOLTAGE) >> 8);
  sendMessage(3, abs(swmVoltage - BOTH_BUTTONS_VOLTAGE) & 0xFF, abs(swmVoltage - BOTH_BUTTONS_VOLTAGE) >> 8);
  ButtonPairStates mButtonStates = voltageToButtonStates(swmVoltage);
  sendMessage(DEBUG_BOOT, mButtonStates.firstButtonState ? 1 : 0, mButtonStates.secondButtonState ? 1 : 0);
  sendMessage(5, switchStates, previousSwitchStates);
  if (previousSwitchStates != switchStates) {
    #if USART_DEBUG_ENABLED
    Serial.print("SWITCHES: ");
    Serial.println(switchStates);
    #endif
    byte changed = previousSwitchStates ^ switchStates;
    previousSwitchStates = switchStates;
    #if USART_DEBUG_ENABLED
    Serial.print("Changed: ");
    Serial.println(changed);
    #endif
    if (changed) {
      // TODO: Do not use SW_INTS for mask on PCB version 3
      for (byte i = 0; i < 3; ++i) {
        byte switchMask = (1 << SW_INTS[i]);
        if (changed & switchMask) {
          handleButtonChange(i, (switchStates & switchMask) ? 0 : 1);
        }
      }
    }
  }
#endif

#if PCB_VERSION != 3 // TODO
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_TOUCH)
  if (previousTouchStates != touchStates) {
    #if USART_DEBUG_ENABLED
    Serial.print("TOUCHES: ");
    Serial.println(touchStates);
    #endif
    byte changed = previousTouchStates ^ touchStates;
    previousTouchStates = touchStates;
    #if USART_DEBUG_ENABLED
    Serial.print("Changed: ");
    Serial.println(changed);
    #endif
    // TODO:
    if (changed) {
      for (byte i = 0; i < 3; ++i) {
        byte switchMask = (1 << SW_INTS[i]);
        if (changed & switchMask) {
          handleButtonChange(i, (switchStates & switchMask) ? 0 : 1);
        }
      }
    }
  }
#endif
#endif

#if PCB_VERSION != 3 // TODO
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
  for (byte board = 0; board < 3; board++) {
    const byte padStateIndex = board - 1;
    const byte boardPadStates = padStates[padStateIndex];
    const byte previousBoardPadStates = previousPadStates[padStateIndex];
    if (previousBoardPadStates != boardPadStates) {
      #if USART_DEBUG_ENABLED
      Serial.print("BOARD: ");
      Serial.println(board);
      #endif
      byte changed = previousBoardPadStates ^ boardPadStates;
      previousPadStates[padStateIndex] = boardPadStates;
      #if USART_DEBUG_ENABLED
      Serial.print("Changed: ");
      Serial.println(changed);
      #endif
      if (changed) {
        for (byte i = 0; i < 4; ++i) {
          byte padMask = (1 << i);
          if (changed & padMask) {
            byte pinState = (boardPadStates & padMask) ? 1 : 0;
            noInterrupts();
            pixels.setPixelColor((board == BOARD_L1 ? 4 : 0) + i, pixels.Color(0, pinState ? 0 : 70, 0));
            pixels.show();
            interrupts();
            handleButtonChange(i, pinState);
          }
        }
      }
    }
  }
#endif
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_ENCODER) || ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_POT) // TODO: separate encoder and pot?
  for (int i = 0; i < BOARD_COUNT; ++i) {
    int position;
    byte positionChanged = false;
    
    if (BOARD_FEATURES[i] & BOARD_FEATURE_ENCODER) {
      position = (*encoders[i]).getPosition(); // TODO: use getDirection or create separate relative encoder feature
      positionChanged = position != positions[i];
      
#if USART_DEBUG_ENABLED
      byte stateA = digitalRead(ENCODER_PINS[i][0]);
      byte stateB = digitalRead(ENCODER_PINS[i][1]);
  
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
#if PCB_VERSION != 3
    } else if (BOARD_FEATURES[i] & BOARD_FEATURE_POT) {
      positionChanged = position != positions[i] && (position == 0 || position == 127 || POT_CHANGE_THRESHOLD < abs(positions[i] - position));
      // Resolution restricted to 7-bits for MIDI compatibility
      position = analogRead(POT_PINS[i]) >> 3;
#endif
    } else {
      continue;
    }
    
    if (positionChanged) {
      handlePositionChange(i, position);
      positions[i] = position;
    }
  }
  #endif

  #if USART_DEBUG_ENABLED
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
  toggleBuiltinLed();
  Wire.beginTransmission(1);
  byte message[] = {address, input, type, value};
  Wire.write(message, 4);
  Wire.endTransmission();
}

#if PCB_VERSION != 3 // TODO
inline byte readPadPin(byte board, byte pin) {
  Serial.print("Reading: ");
  Serial.println(PAD_PINS[board][pin]);
  Serial.print("Value: ");
  Serial.println(digitalRead(PAD_PINS[board][pin]));
  return (digitalRead(PAD_PINS[board][pin]) == LOW ? 0 : 1) << pin;
}

inline void updatePadStates() {
  for (byte board = 1; board < 4; ++board) { // Pads and buttons not available on leftmost and rightmost boards
    if (BOARD_FEATURES[board] & BOARD_FEATURE_PADS) {
      const byte padStateIndex = board - 1;
      padStates[padStateIndex] = readPadPin(board, 3) | readPadPin(board, 2) | readPadPin(board, 1) | readPadPin(board, 0);
      #if USART_DEBUG_ENABLED
      Serial.print("BOARD PADS ");
      Serial.println(board);
      Serial.print("States: ");
      Serial.println(padStates[padStateIndex]);
      #endif
    }
  }
}
#endif

inline void updateSwitchStates() { // TODO: this is not called when the second button goes down because the logical state of the pin does not change
#if PCB_VERSION == 3
  switchStates = getButtonStates();
#else
  switchStates = PINC &
    ((!!(BOARD_FEATURES[1] & BOARD_FEATURE_BUTTON) << SW_INTS[1]) |
    (!!(BOARD_FEATURES[2] & BOARD_FEATURE_BUTTON) << SW_INTS[2]) |
    (!!(BOARD_FEATURES[3] & BOARD_FEATURE_BUTTON) << SW_INTS[3]));
#endif
}

#if PCB_VERSION != 3 // TODO
inline void updateTouchStates() {
  for (byte i = 1; i < 4; ++i) { // Pads and buttons not available on leftmost and rightmost boards
    if (BOARD_FEATURES[i] & BOARD_FEATURE_TOUCH) {
      touchStates |= TOUCH_PINS[i];
    }
  }
}
#endif

// !!NOTE!!: Do not call sendMessage in ISRs
ISR(PCINT0_vect) {
#if USART_DEBUG_ENABLED
  interrupter = 0;
#endif

#if HAS_FEATURE(R2, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_R2]).tick();
#endif

#if PCB_VERSION == 3

#if HAS_FEATURE(M2, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_M2]).tick();
#endif

#else

#if HAS_FEATURE(R1, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_R1]).tick();
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
  updatePadStates();
#endif

#endif
}

// !!NOTE!!: Do not call sendMessage in ISRs
ISR(PCINT1_vect) {
#if USART_DEBUG_ENABLED
  interrupter = 1;
#endif

#if PCB_VERSION == 3

#if HAS_FEATURE(L1, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_L1]).tick();
#endif

#else

#if HAS_FEATURE(M, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_M]).tick();
#endif
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
  updatePadStates();
#endif

#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
  updateSwitchStates();
#endif
}

// !!NOTE!!: Do not call sendMessage in ISRs
ISR(PCINT2_vect) {
#if USART_DEBUG_ENABLED
  interrupter = 2;
#endif

#if PCB_VERSION == 3

#if HAS_FEATURE(M1, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_M1]).tick();
#endif
#if HAS_FEATURE(L2, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_M2]).tick();
#endif
#if HAS_FEATURE(R1, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_R1]).tick();
#endif

#else

#if HAS_FEATURE(M, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_M]).tick();
#endif
#if HAS_FEATURE(L1, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_L1]).tick();
#endif
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
  updatePadStates();
#endif

#endif


#ifndef USART_DEBUG_ENABLED
#if HAS_FEATURE(L2, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_L2]).tick();
#endif
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
  updateSwitchStates();
#endif

#if PCB_VERSION != 3 // TODO
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_TOUCH)
  updateTouchStates();
#endif
#endif
}
