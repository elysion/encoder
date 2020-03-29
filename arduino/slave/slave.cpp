#include <EEPROM.h>
#include <Wire.h>

#include "slave.h"
#include "features.h"
#include "interrupts.h"
// TODO: if this is not imported here, the initialization will fail and the device will not work properly
// TODO: this should be fixed in order to be able to use this code as a library
#include "config.h"

#if PCB_VERSION == 3
#define LED_BUILTIN_AVAILABLE (BOARD_FEATURES_M2 == NO_FEATURES) // TODO: only disable when R2 uses encoder? (How does the pull-up on the pin affect this need?)
#else
#define LED_BUILTIN_AVAILABLE (BOARD_FEATURES_R2 == NO_FEATURES) // TODO: only disable when R2 uses encoder? (How does the pull-up on the pin affect this need?)
#endif

#ifdef INTERRUPT_DEBUG
uint8_t interrupter;
#endif

void(* reset) (void) = 0;

inline void togglePin(uint8_t outputPin) {
  digitalWrite(outputPin, !digitalRead(outputPin));
}

bool isInRange(int value, int target, int range) {
  return value > target - range && value < target + range;
}

Slave_::Slave_() {
  #if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
  padStates = {
    0b00001111,
    0b00001111,
    0b00001111
  };

  previousPadStates = {
    0b00001111,
    0b00001111,
    0b00001111
  };
  #endif

  #ifdef INTERRUPT_DEBUG
  interrupter = 255;
  #endif
}

void Slave_::setup(ChangeHandler changeHandler) {
  handler = changeHandler;

#if HAS_FEATURE(L1, BOARD_FEATURE_LED) || HAS_FEATURE(L2, BOARD_FEATURE_LED)
  leds[0] = new Adafruit_NeoPixel(12, LEDL, NEO_GRB + NEO_KHZ800);
  leds[0]->begin();
#endif
#if HAS_FEATURE(M1, BOARD_FEATURE_LED) || HAS_FEATURE(M2, BOARD_FEATURE_LED) || HAS_FEATURE(M, BOARD_FEATURE_LED)
  leds[1] = new Adafruit_NeoPixel(12, LEDM, NEO_GRB + NEO_KHZ800);
  leds[1]->begin();
#endif
#if HAS_FEATURE(R1, BOARD_FEATURE_LED) || HAS_FEATURE(R2, BOARD_FEATURE_LED)
  leds[2] = new Adafruit_NeoPixel(12, LEDR, NEO_GRB + NEO_KHZ800);
  leds[2]->begin();
#endif
  
  if (LED_BUILTIN_AVAILABLE) {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
  }

  delay(10);
  setupI2c();

  #ifndef USART_DEBUG_ENABLED
  Serial.end();
  #endif

  setupPinModes();
  // TODO: Move interrupt initializations to the loop in setupPinModes();
  setupInterrupts();

  #if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
  // TODO: initialize according to enabled buttons
  switchStates = previousSwitchStates = getButtonStates(); // TODO: construct mask according to enabled buttons
  #endif
  // TODO: initialize touch states
}

void Slave_::update() {
  #ifdef PORT_STATE_DEBUG
  uint8_t maskedPinC = PINC; // & 0x00001111;
  if (previousB != PINB) {
    sendMessage(1, PINB, 100);
  }
  if (previousC != maskedPinC) {
    sendMessage(2, PINC, 100);
  }
  if (previousD != PIND) {
    sendMessage(3, PIND, 100);
  }

  previousB = PINB;
  previousC = maskedPinC;
  previousD = PIND;
  return;
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_MATRIX)
  for (uint8_t board = BOARD_L1; board <= BOARD_R1; ++board) {
    if (BOARD_FEATURES[board] & BOARD_FEATURE_MATRIX) {
      uint8_t boardMatrixIndex = BOARD_MATRIX_INDEX(board);
      for (uint8_t output = 0; output < MATRIX_OUTPUTS; ++output) {
        uint8_t inputStates = 0;
        uint8_t previousInputStates = previousMatrixButtonStates[boardMatrixIndex][output];
        digitalWrite(BUTTON_MATRIX_OUTPUT_PINS[boardMatrixIndex][output], LOW);
        delay(10); // TODO: is this necessary?
        for (uint8_t input = 0; input < MATRIX_INPUTS; ++input) {
          uint8_t currentState = digitalRead(BUTTON_MATRIX_INPUT_PINS[boardMatrixIndex][input]) == HIGH ? 0 : 1;
          if (currentState != bitRead(previousInputStates, input)) {
            // TODO: this will conflict with button on M / M1 & M2
            // TODO: use MATRIX instead of BUTTON
            handler(CONTROL_TYPE_BUTTON, board, MATRIX_INPUTS * output + input, currentState)
          }
          bitWrite(inputStates, input, currentState);
        }
        digitalWrite(BUTTON_MATRIX_OUTPUT_PINS[boardMatrixIndex][output], HIGH);
        previousMatrixButtonStates[boardMatrixIndex][output] = inputStates;
      }
    }
  }
#endif

  // TODO: check touch
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
//  delay(500);
//  int swmVoltage = analogRead(SWM);
//  sendMessage(1, abs(swmVoltage - FIRST_BUTTON_VOLTAGE) & 0xFF, abs(swmVoltage - FIRST_BUTTON_VOLTAGE) >> 8);
//  sendMessage(2, abs(swmVoltage - SECOND_BUTTON_VOLTAGE) & 0xFF, abs(swmVoltage - SECOND_BUTTON_VOLTAGE) >> 8);
//  sendMessage(3, abs(swmVoltage - BOTH_BUTTONS_VOLTAGE) & 0xFF, abs(swmVoltage - BOTH_BUTTONS_VOLTAGE) >> 8);
//  ButtonPairStates mButtonStates = voltageToButtonStates(swmVoltage);
//  sendMessage(DEBUG_BOOT, mButtonStates.firstButtonState ? 1 : 0, mButtonStates.secondButtonState ? 1 : 0);
//  sendMessage(5, switchStates, previousSwitchStates);
  if (previousSwitchStates != switchStates) {
    #ifdef USART_DEBUG_ENABLED
    Serial.print("SWS: ");
    Serial.println(switchStates);
    #endif
    uint8_t changed = previousSwitchStates ^ switchStates;
    #ifdef USART_DEBUG_ENABLED
    Serial.print("Ch: ");
    Serial.println(changed);
    #endif
    if (changed) {
      previousSwitchStates = switchStates;
      // TODO: Do not use SW_INTS for mask on PCB version 3
      for (uint8_t i = BOARD_L1; i <= BOARD_R1; ++i) {
        uint8_t switchMask = (1 << SW_INTS[i]);
        if (changed & switchMask) {
          handler(i, CONTROL_TYPE_BUTTON, 0, (switchStates & switchMask) ? 0 : 1);
        }
      }
    }
  }
#endif

#if PCB_VERSION != 3 // TODO
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_TOUCH)
  if (previousTouchStates != touchStates) {
    #ifdef USART_DEBUG_ENABLED
    Serial.print("TS: ");
    Serial.println(touchStates);
    #endif
    uint8_t changed = previousTouchStates ^ touchStates;
    previousTouchStates = touchStates;
    #ifdef USART_DEBUG_ENABLED
    Serial.print("Ch: ");
    Serial.println(changed);
    #endif
    // TODO:
    if (changed) {
      for (uint8_t i = 0; i < 3; ++i) {
        uint8_t switchMask = (1 << SW_INTS[i]);
        if (changed & switchMask) {
          handler(i, CONTROL_TYPE_TOUCH, 0, (switchStates & switchMask) ? 0 : 1);
        }
      }
    }
  }
#endif
#endif

#if PCB_VERSION != 3 // TODO
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
  for (uint8_t board = 0; board < 3; board++) {
    const uint8_t padStateIndex = board - 1;
    const uint8_t boardPadStates = padStates[padStateIndex];
    const uint8_t previousBoardPadStates = previousPadStates[padStateIndex];
    if (previousBoardPadStates != boardPadStates) {
      #ifdef USART_DEBUG_ENABLED
      Serial.print("B: ");
      Serial.println(board);
      #endif
      uint8_t changed = previousBoardPadStates ^ boardPadStates;
      previousPadStates[padStateIndex] = boardPadStates;
      #ifdef USART_DEBUG_ENABLED
      Serial.print("Ch: ");
      Serial.println(changed);
      #endif
      if (changed) {
        for (uint8_t i = 0; i < 4; ++i) {
          uint8_t padMask = (1 << i);
          if (changed & padMask) {
            uint8_t pinState = (boardPadStates & padMask) ? 1 : 0;
            // TODO: the leds should be updated in the slave.ino file?
            // setLedColor(board, i, leds.Color(0, pinState ? 0 : 70, 0));
            // showLeds(board);
            // TODO: add type for pad -> easier to tell button events apart in the handler
            handler(board, CONTROL_TYPE_BUTTON, i, pinState);
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
    uint8_t positionChanged = false;
    
    if (BOARD_FEATURES[i] & BOARD_FEATURE_ENCODER) {
      if (ENCODER_TYPES[i] == ENCODER_TYPE_ABSOLUTE) {
        position = (*(encoders)[i]).getPosition();
        positionChanged = position != positions[i];
      } else {
        position = (*(encoders)[i]).getDirection();
        positionChanged = position != 0;
      }
      
#if defined(USART_DEBUG_ENABLED) && defined(INTERRUPT_DEBUG)
      uint8_t stateA = digitalRead(ENCODER_PINS[i][0]);
      uint8_t stateB = digitalRead(ENCODER_PINS[i][1]);
  
      if (stateA != states[2*i] || stateB != states[2*i+1]) {
        Serial.print("Int: ");
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
      // Resolution restricted to 7-bits for MIDI compatibility
      position = analogRead(POT_PINS[i]) >> 3;
      positionChanged = position != positions[i] && (position == 0 || position == 127 || POT_CHANGE_THRESHOLD < abs(positions[i] - position));
#endif
    } else {
      continue;
    }
    
    if (positionChanged) {      
      if (ENCODER_TYPES[i] == ENCODER_TYPE_ABSOLUTE) {
        positions[i] = position;
        handler(i, CONTROL_TYPE_POSITION, 0, position);
      } else {
        handler(i, CONTROL_TYPE_ENCODER, 0, position);
      }
    }
  }
  #endif

  #ifdef USART_DEBUG_ENABLED
  if (Serial.available()) {        // If anything comes in Serial,
    Serial.write(Serial.read());   // read it and send it out
  }
  #endif
}

void Slave_::sendMessage(uint8_t input, uint8_t value, ControlType type) {
  toggleBuiltinLed();
  Wire.beginTransmission(1);
  uint8_t message[] = {address, input, type, value};
  Wire.write(message, 4);
  Wire.endTransmission();
}

void Slave_::toggleBuiltinLed() {
#if PCB_VERSION == 3 && LED_BUILTIN_AVAILABLE
    togglePin(LED_BUILTIN);
#endif
}

void Slave_::tickEncoder(Board board) {
  (*(encoders)[board]).tick();
}

inline void Slave_::setupInterrupts() {
  PCICR |= (1 << PCIE0) | (1 << PCIE1) | (1 << PCIE2);

  if (BOARD_FEATURES[BOARD_L2] & BOARD_FEATURE_ENCODER) {
    enablePCINT(ENCL2A);
    enablePCINT(ENCL2B);
  }

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

inline void Slave_::setupPinModes() {
  for (uint8_t i = 0; i < BOARD_COUNT; ++i) {
    const uint8_t boardFeatures = BOARD_FEATURES[i];

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_ENCODER)
    if (boardFeatures & BOARD_FEATURE_ENCODER) {
      pinMode(ENCODER_PINS[i][0], INPUT_PULLUP);
      pinMode(ENCODER_PINS[i][1], INPUT_PULLUP);
    }
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_MATRIX)
    if (boardFeatures & BOARD_FEATURE_MATRIX) {
      for (uint8_t output = 0; output < MATRIX_OUTPUTS; ++output) {
        uint8_t pin = BUTTON_MATRIX_OUTPUT_PINS[BOARD_MATRIX_INDEX(i)][output];
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
      }
      for (uint8_t input = 0; input < MATRIX_INPUTS; ++input) {
        sendMessage(DEBUG_BOOT + 2, BOARD_MATRIX_INDEX(i), input);
        pinMode(BUTTON_MATRIX_INPUT_PINS[BOARD_MATRIX_INDEX(i)][input], INPUT_PULLUP);
      }
    }
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
    if (boardFeatures & BOARD_FEATURE_BUTTON) {
#if PCB_VERSION == 3
      pinMode(BUTTON_PINS[i], INPUT);
#else
      pinMode(BUTTON_PINS[i], INPUT_PULLUP);
#endif      
    }
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_POT)
    if (boardFeatures & BOARD_FEATURE_POT) {
      // TODO: anything needed here?
    }
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_TOUCH)
#if PCB_VERSION != 3 // TODO
    if (boardFeatures & BOARD_FEATURE_TOUCH) {
      pinMode(TOUCH_PINS[i], INPUT);
    }
#endif
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
#if PCB_VERSION != 3 // TODO
    if (BOARD_FEATURES[i] & BOARD_FEATURE_PADS) {
      for (uint8_t j = 0; j < 4; ++j) {
        #ifdef USART_DEBUG_ENABLED
        Serial.print("Configuring pin as input: ");
        Serial.println(PAD_PINS[i][j]);
        #endif
        pinMode(PAD_PINS[i][j], INPUT_PULLUP);
      }
    } 
#endif
#endif
  }
}

inline void Slave_::setupI2c() {
  address = EEPROM.read(0);
  Serial.println("Boot");
  Serial.print("A: ");
  Serial.println(address);
  
 if (address == 255 || address < 10) {
    Serial.print("A: ");
    Serial.println(address);
    Serial.println("Req addr from master");
    Wire.begin();
    Wire.requestFrom(1, 1);
    while (Wire.available()) {
      address = Wire.read();
      Serial.print("Got addr: ");
      Serial.println(address);
    }

    if (address == 255) {
      Serial.println("Did not get addr. Reset.");
      delay(1000);
      reset();
    }
  
    Serial.print("Start w/ addr: ");
    Serial.println(address);
    Wire.begin(address);

    EEPROM.write(0, address);
  
    delay(900);
    sendMessage(DEBUG_RECEIVED_ADDRESS, address, CONTROL_TYPE_DEBUG);
  } else {
    Wire.begin(address);
    sendMessage(DEBUG_BOOT, 1, CONTROL_TYPE_DEBUG);
  }

  Serial.println("Done");
}

#if PCB_VERSION != 3 // TODO
inline uint8_t Slave_::readPadPin(uint8_t board, uint8_t pin) {
  Serial.print("Read: ");
  Serial.println(PAD_PINS[board][pin]);
  Serial.print("Val: ");
  Serial.println(digitalRead(PAD_PINS[board][pin]));
  return (digitalRead(PAD_PINS[board][pin]) == LOW ? 0 : 1) << pin;
}

inline void Slave_::updatePadStates() {
  for (uint8_t board = 1; board < 4; ++board) { // Pads and buttons not available on leftmost and rightmost boards
    if (BOARD_FEATURES[board] & BOARD_FEATURE_PADS) {
      const uint8_t padStateIndex = board - 1;
      padStates[padStateIndex] = readPadPin(board, 3) | readPadPin(board, 2) | readPadPin(board, 1) | readPadPin(board, 0);
      #ifdef USART_DEBUG_ENABLED
      Serial.print("BOARD PADS ");
      Serial.println(board);
      Serial.print("States: ");
      Serial.println(padStates[padStateIndex]);
      #endif
    }
  }
}
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
void Slave_::updateSwitchStates() { // TODO: this is not called when the second button goes down because the logical state of the pin does not change
#if PCB_VERSION == 3
  switchStates = getButtonStates();
#else
  switchStates = SWITCH_PORT &
    ((!!(BOARD_FEATURES[BOARD_L1] & BOARD_FEATURE_BUTTON) << SW_INTS[BOARD_L1]) |
    (!!(BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_BUTTON) << SW_INTS[BOARD_M]) |
    (!!(BOARD_FEATURES[BOARD_R1] & BOARD_FEATURE_BUTTON) << SW_INTS[BOARD_R1]));
#endif
}
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_TOUCH)
#if PCB_VERSION != 3 // TODO
void Slave_::updateTouchStates() {
  for (uint8_t i = 1; i < 4; ++i) { // Pads and buttons not available on leftmost and rightmost boards
    if (BOARD_FEATURES[i] & BOARD_FEATURE_TOUCH) {
      touchStates |= TOUCH_PINS[i];
    }
  }
}
#endif
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
ButtonPairStates Slave_::voltageToButtonStates(int voltage) {
  ButtonPairStates buttonStates;
  bool bothButtonsPressed = isInRange(voltage, BOTH_BUTTONS_VOLTAGE, BUTTON_VOLTAGE_RANGE);
  buttonStates.firstButtonState = bothButtonsPressed || isInRange(voltage, FIRST_BUTTON_VOLTAGE, BUTTON_VOLTAGE_RANGE);
  buttonStates.secondButtonState = bothButtonsPressed || isInRange(voltage, SECOND_BUTTON_VOLTAGE, BUTTON_VOLTAGE_RANGE);
  return buttonStates;
}
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
uint8_t Slave_::getButtonStates() {
#if PCB_VERSION == 3
  uint8_t buttonStates = 0;
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

  return buttonStates;
#else
  return PINC & SW_INTS_MASK;
#endif
}
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_LED)
void Slave_::setLedColor(Board board, uint16_t position, uint16_t color) {
  ledsForBoard(board)->setPixelColor(position, color);
}

void Slave_::fillLeds(Board board, uint16_t color, uint16_t first = 0, uint16_t count = 0) {
  ledsForBoard(board)->fill(color, first, count);
}

void Slave_::showLeds(Board board) {
 ledsForBoard(board)->show();
}

Adafruit_NeoPixel* Slave_::ledsForBoard(Board board) {
  switch (board) {
    case BOARD_L1:
    case BOARD_L2:
      return leds[0];
    #if PCB_VERSION == 3
    case BOARD_M1:
    case BOARD_M2:
    #else
    case BOARD_M:
    #endif
      return leds[1];
    case BOARD_R1:
    case BOARD_R2:
      return leds[2];
  }
}

#endif

Slave_ Slave;
