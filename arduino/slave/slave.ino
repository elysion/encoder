#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Wire.h>
#include <EEPROM.h>

#include "config.h"
#include "shared.h"
#include "slave.h"

// TODO: do not include pixel strip objects if there are no leds
Adafruit_NeoPixel led1Pixels(LED_COUNT_LM, LED1, NEO_GRB + NEO_KHZ800); // TODO set led count correctly
Adafruit_NeoPixel led2Pixels(LED_COUNT_R, LED2, NEO_GRB + NEO_KHZ800); // TODO set led count correctly

volatile byte address;

byte switchStates = 0;
byte previousSwitchStates = 0;

byte touchStates;
byte previousTouchStates;

byte ticks = 0;

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

const byte BOARD_FEATURES[] = {
  BOARD_FEATURES_L2,
  BOARD_FEATURES_L1,
  BOARD_FEATURES_M,
  BOARD_FEATURES_R1,
  BOARD_FEATURES_R2,
};

#include "feature_validation.h"

RotaryEncoder* encoders[5] = {
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
  #if BOARD_FEATURES_M & BOARD_FEATURE_ENCODER
  new RotaryEncoder(ENCODER_PINS[BOARD_M][0], ENCODER_PINS[BOARD_M][1])
  #else
  0
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

long positions[] = {
  0, // TODO: is it wise to initialize these to 0?
  0,
  0,
  0, 
  0
};

#if defined(USART_DEBUG_ENABLED) || defined(I2C_DEBUG_ENABLED)
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
  //TODO: check that boards have leds!
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  #if LED_COUNT_LM != 0
  led1Pixels.begin();
  led1Pixels.fill(led1Pixels.ColorHSV(0, 255, 255), 0);
  led1Pixels.show();
  #endif
  #if LED_COUNT_R != 0
  led2Pixels.begin();
  #endif

  // TODO: validate board feature combinations
  address = EEPROM.read(0);

  #ifdef USART_DEBUG_ENABLED
  Serial.begin(115200);
  Serial.println("Boot");
  Serial.print("Address: ");
  Serial.println(address);
  Serial.end();
  #endif
  
 if (address == 255 || address < 10) {
    #ifdef USART_DEBUG_ENABLED
    Serial.print("Address: ");
    Serial.println(address);
    Serial.println("Requesting address from master");
    #endif
    
    Wire.begin();
    Wire.requestFrom(1, 1);
    while (Wire.available()) {
      address = Wire.read();
      #ifdef USART_DEBUG_ENABLED
      Serial.print("Received address: ");
      Serial.println(address);
      #endif
    }

    if (address == 255) {
      #ifdef USART_DEBUG_ENABLED
      Serial.println("Did not receive address from master. Resetting.");
      #endif
      reset();
    }

    #ifdef USART_DEBUG_ENABLED
    Serial.print("Starting with address: ");
    Serial.println(address);
    #endif
    Wire.begin(address);

    EEPROM.write(0, address);
  
    delay(900);
    sendMessage(DEBUG_RECEIVED_ADDRESS, address, CONTROL_TYPE_DEBUG);
  } else {
    Wire.begin(address);
    sendMessage(DEBUG_BOOT, 255, CONTROL_TYPE_DEBUG);
  }
  sendMessage(DEBUG_BOOT, 254, CONTROL_TYPE_DEBUG);

  for (byte i = 0; i < 5; ++i) {
    sendMessage(DEBUG_BOOT, 253, CONTROL_TYPE_DEBUG);
    const byte boardFeatures = BOARD_FEATURES[i];

    if (boardFeatures & BOARD_FEATURE_ENCODER) {
      pinMode(ENCODER_PINS[i][0], INPUT_PULLUP);
      pinMode(ENCODER_PINS[i][1], INPUT_PULLUP);
    }

    if (boardFeatures & BOARD_FEATURE_BUTTON) {
      pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    }

    if (boardFeatures & BOARD_FEATURE_POT) {
      // TODO: anything needed here?
    }
    
    if (boardFeatures & BOARD_FEATURE_TOUCH) {
      pinMode(TOUCH_PINS[i], INPUT);
    }

    if (BOARD_FEATURES[i] & BOARD_FEATURE_PADS) {
      for (byte j = 0; j < 4; ++j) {
        #ifdef USART_DEBUG_ENABLED
        Serial.print("Configuring pin as input: ");
        Serial.println(PAD_PINS[i][j]);
        #endif
        #ifdef I2C_DEBUG_ENABLED
        //sendMessage(0, PAD_PINS[i][j], CONTROL_TYPE_DEBUG);
        #endif
        pinMode(PAD_PINS[i][j], INPUT_PULLUP);
      }
    }
  }

  // TODO: Move interrupt initializations to loop above
  PCICR |= (1 << PCIE0) | (1 << PCIE1) | (1 << PCIE2);

  // TODO: use enablePCINT
  if (BOARD_FEATURES[BOARD_L2] & BOARD_FEATURE_ENCODER) {
#if PCB_VERSION == 1
    enablePCINT(ENCL2A);
    enablePCINT(ENCL2B);
#elif PCB_VERSION == 2
    PCMSK2 |= (1 << ENCL2A_INT) | (1 << ENCL2B_INT);
#else
static_assert(false, "Unknown PCB")
#endif
  }

  if (BOARD_FEATURES[BOARD_L1] & BOARD_FEATURE_ENCODER) {
    enablePCINT(ENCL1A);
    enablePCINT(ENCL1B);
  }

  if (BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_ENCODER) {
    // TODO: generalize this so that you can use INT -> PCMSK. How about enablePCINT?
#if PCB_VERSION == 1
    enablePCINT(ENC1A);
    enablePCINT(ENC1B);
#elif PCB_VERSION == 2
    PCMSK1 |= (1 << ENC1B_INT);
    PCMSK2 |= (1 << ENC1A_INT);
#else
static_assert(false, "Unknown PCB")
#endif
  }
  
  if (BOARD_FEATURES[BOARD_R1] & BOARD_FEATURE_ENCODER) {
    enablePCINT(ENCR1A);
    enablePCINT(ENCR1B);
  }

  if (BOARD_FEATURES[BOARD_R2] & BOARD_FEATURE_ENCODER) {
#if PCB_VERSION == 1
    enablePCINT(ENCR2A);
    enablePCINT(ENCR2B);
#elif PCB_VERSION == 2
    PCMSK0 |= (1 << ENCR2A_INT) | (1 << ENCR2B_INT);
#else
    static_assert(false, "Unknown PCB")
#endif
  }

  if (BOARD_FEATURES[BOARD_L1] & (BOARD_FEATURE_BUTTON | BOARD_FEATURE_TOUCH)) {
    enablePCINT(SWL);
  }
    
  if (BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_BUTTON) {
    enablePCINT(SW1);
  }

  if (BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_TOUCH) {
    enablePCINT(TOUCH);
  }
    
  if (BOARD_FEATURES[BOARD_R1] & (BOARD_FEATURE_BUTTON | BOARD_FEATURE_TOUCH)) {
    enablePCINT(SWR);
  }

  if (BOARD_FEATURES[BOARD_L1] & BOARD_FEATURE_PADS) {
    enablePCINT(SWL);
    enablePCINT(ENCL1A);
    enablePCINT(ENCL1B);
    enablePCINT(ENCL2A);
  }

  if (BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_PADS) {
    enablePCINT(SW1);
    enablePCINT(ENC1B);
    enablePCINT(ENC1A);
    enablePCINT(TOUCH); // TODO: fix  POT -> TOUCH on board
  }
  
  if (BOARD_FEATURES[BOARD_R1] & BOARD_FEATURE_PADS) {
    enablePCINT(SWR);
    enablePCINT(ENCR1B);
    enablePCINT(ENCR1A);
    enablePCINT(ENCR2A);
  }

  // TODO: initialize according to enabled buttons
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
  updateSwitchStates();
  previousSwitchStates = switchStates;
#endif
  // TODO: initialize touch states
}

byte portB = PINB;
byte portC = PINC;
byte portD = PIND;

inline void printPortStateChanges() {
  #if defined(I2C_DEBUG_ENABLED) && defined(PORT_STATE_DEBUG)
  byte maskedB = PINB & 0b11000011;
  byte maskedC = PINC & 0b11001111;
  byte maskedD = PIND & 0b11111111;
  if (portB != maskedB) {
    sendMessage(8, maskedB, CONTROL_TYPE_DEBUG);
    portB = maskedB;
  }
  if (portC != maskedC) {
    sendMessage(9, maskedC, CONTROL_TYPE_DEBUG);
    portC = maskedC;
  }
  if (portD != maskedD) {
    sendMessage(10, maskedD, CONTROL_TYPE_DEBUG);
    portD = maskedD;
  }
  #endif
}

inline void printEncoderStates(byte i) {
  byte stateA = digitalRead(ENCODER_PINS[i][0]);
  byte stateB = digitalRead(ENCODER_PINS[i][1]);

#if defined(USART_DEBUG_ENABLED) || defined(I2C_DEBUG_ENABLED)
  if (stateA != states[2*i] || stateB != states[2*i+1]) {
    #if defined(INTERRUPT_DEBUG) && defined(USART_DEBUG_ENABLED)
    Serial.print("Interrupter: ");
    Serial.println(interrupter);
    Serial.print("Pins ");
    Serial.println(i);
    Serial.println("A B");
    Serial.print(stateA);
    Serial.print(" ");
    Serial.println(stateB);
    #endif
    
    #if defined(INTERRUPT_DEBUG) && defined(I2C_DEBUG_ENABLED)
    sendMessage(3, stateA, CONTROL_TYPE_DEBUG);
    sendMessage(3, stateB, CONTROL_TYPE_DEBUG);
    sendMessage(4, interrupter, CONTROL_TYPE_DEBUG);
    sendMessage(7, ticks, CONTROL_TYPE_DEBUG);
    #endif
    
    states[2*i] = stateA;
    states[2*i+1] = stateB;
  }
#endif
}

void loop() {
#if defined(USART_DEBUG_ENABLED) || defined(I2C_DEBUG_ENABLED)
  printPortStateChanges();
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
  if (previousSwitchStates != switchStates) {
    #ifdef USART_DEBUG_ENABLED
    Serial.print("SWITCHES: ");
    Serial.println(switchStates);
    #endif
    #ifdef I2C_DEBUG_ENABLED
    sendMessage(11, switchStates, CONTROL_TYPE_DEBUG);
    #endif
    byte changed = previousSwitchStates ^ switchStates;
    previousSwitchStates = switchStates;
    #ifdef USART_DEBUG_ENABLED
    Serial.print("Changed: ");
    Serial.println(changed);
    #endif
    #ifdef I2C_DEBUG_ENABLED
    sendMessage(12, changed, CONTROL_TYPE_DEBUG);
    #endif
    if (changed) {
      for (byte i = BOARD_L1; i < BOARD_R2; ++i) {
        byte switchMask = (1 << SW_INTS[i]);
        if (changed & switchMask) {
          handleButtonChange(i, (switchStates & switchMask) ? 0 : 1);
        }
      }
    }
  }
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_TOUCH)
  if (previousTouchStates != touchStates) {
    #ifdef USART_DEBUG_ENABLED
    Serial.print("TOUCHES: ");
    Serial.println(touchStates);
    #endif
    byte changed = previousTouchStates ^ touchStates;
    previousTouchStates = touchStates;
    #ifdef USART_DEBUG_ENABLED
    Serial.print("Changed: ");
    Serial.println(changed);
    #endif
    if (changed) {
      for (byte i = BOARD_L1; i < BOARD_R2; ++i) {
        byte switchMask = (1 << SW_INTS[i]);
        if (changed & switchMask) {
          handleButtonChange(i, (switchStates & switchMask) ? 0 : 1);
        }
      }
    }
  }
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
  for (byte board = BOARD_L1; board < BOARD_R2; board++) {
    const byte padStateIndex = board - 1;
    const byte boardPadStates = padStates[padStateIndex];
    const byte previousBoardPadStates = previousPadStates[padStateIndex];
    if (previousBoardPadStates != boardPadStates) {
      #ifdef USART_DEBUG_ENABLED
      Serial.print("BOARD: ");
      Serial.println(board);
      #endif
      byte changed = previousBoardPadStates ^ boardPadStates;
      previousPadStates[padStateIndex] = boardPadStates;
      #ifdef USART_DEBUG_ENABLED
      Serial.print("Changed: ");
      Serial.println(changed);
      #endif
      if (changed) {
        for (byte i = 0; i < 4; ++i) {
          byte padMask = (1 << i);
          if (changed & padMask) {
            byte pinState = (boardPadStates & padMask) ? 1 : 0;
            // TODO: only set live according to messages received from master
            //noInterrupts(); // TODO: is interrupt disabling necessary?

            /* TODO: check that boards have leds before running this / update leds only according to incoming messages / update only for absolute encoders?
            if (board == BOARD_R1) {
              led2Pixels.setPixelColor(i, led2Pixels.Color(0, pinState ? 0 : 70, 0));
              led2Pixels.show();
            } else {
              led1Pixels.setPixelColor((board == BOARD_L1 ? 4 : 0) + i, led1Pixels.Color(0, pinState ? 0 : 70, 0));
              led1Pixels.show();
            }
            //interrupts();
            */
            handleButtonChange((board*4) + i, pinState);
          }
        }
      }
    }
  }
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_ENCODER) || ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_POT) // TODO: separate encoder and pot?
  for (int i = BOARD_L2; i <= BOARD_R2; ++i) {
    long position;
    long positionChange = 0;
    const byte encoderType = ENCODER_TYPES[i];
    
    if (BOARD_FEATURES[i] & BOARD_FEATURE_ENCODER) {
      position = (*encoders[i]).getPosition();
      positionChange = position - positions[i];

      #if ((defined(USART_DEBUG_ENABLED) || defined(I2C_DEBUG_ENABLED)) && defined(ENCODER_PIN_DEBUG))
      printEncoderStates(i);
      #endif
    } else if (BOARD_FEATURES[i] & BOARD_FEATURE_POT) {
      // Resolution restricted to 7-bits for MIDI compatibility
      position = analogRead(POT_PINS[i]) >> 3; // TODO: check that this 10bit -> 7bit conversion actually works
      /* TODO: Update to work with positionChange
      positionChanged = position != positions[i] && (position == 0 || position == 127 || POT_CHANGE_THRESHOLD < abs(positions[i] - position));
      */
    } else {
      continue;
    }

    if (positionChange != 0) {
      if (encoderType == ENCODER_TYPE_RELATIVE) { // TODO: handle pots
        const byte value = positionChange > 0 ? 1 : 255;
        // TODO: is this what is causing the slowness of the sending?
        /*
        if (abs(positionChange) > 10) {
          //Address: 88, Control: 100, Debug: out of bounds!, Value: 255
          sendMessage(100, abs(positionChange), CONTROL_TYPE_DEBUG);
        } else {
          sendMessage(101, abs(positionChange), CONTROL_TYPE_DEBUG);
        }
        */
        //for (byte j = 0; j < constrain(abs(positionChange), 0, 10); ++j) {
          handlePositionChange(i, value);
        //}
      } else {
        sendMessage(200, i, CONTROL_TYPE_DEBUG);
        handlePositionChange(i, position);
        /* TODO: Check that the board have leds / update only according to input / only for absolute encoders
        Adafruit_NeoPixel *pixels = i <= BOARD_M ? (&led1Pixels) : (&led2Pixels);
        (*pixels).fill(0, 0);
        (*pixels).setPixelColor(position, 0, 100, 0, 0);
        (*pixels).show();
        */
      }
      positions[i] = position;
    }
  }
  #endif
  
  #ifdef USART_DEBUG_ENABLED
  if (Serial.available()) {        // If anything comes in Serial,
    Serial.write(Serial.read());   // read it and send it out
  }
  #endif
}

// TODO: remove duplication?
void handleEncoderChange(byte input, byte state) {
  sendMessage(input, state, CONTROL_TYPE_ENCODER);
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

void debugPadPinStates(byte board, byte pin) {
  Serial.print("Reading: ");
  Serial.println(PAD_PINS[board][pin]);
  Serial.print("Value: ");
  Serial.println(digitalRead(PAD_PINS[board][pin]));
}

inline byte readPadPin(byte board, byte pin) {
  #if USART_DEBUG_ENABLED
  debugPadPinStates(board, pin);
  #endif
  return (digitalRead(PAD_PINS[board][pin]) == LOW ? 0 : 1) << pin;
}

inline void updatePadStates() {
  for (byte board = 1; board < 4; ++board) { // Pads and buttons not available on leftmost and rightmost boards
    if (BOARD_FEATURES[board] & BOARD_FEATURE_PADS) {
      #ifdef USART_DEBUG_ENABLED
      Serial.print("BOARD PADS ");
      Serial.println(board);
      #endif
      const byte padStateIndex = board - 1;
      padStates[padStateIndex] = readPadPin(board, 3) | readPadPin(board, 2) | readPadPin(board, 1) | readPadPin(board, 0);
      #ifdef USART_DEBUG_ENABLED
      Serial.print("States: ");
      Serial.println(padStates[padStateIndex]);
      #endif
    }
  }
}

inline void updateSwitchStates() {
  // TODO: use HAS_FEATURE macro?
  switchStates = SWITCH_PORT &
    ((!!(BOARD_FEATURES[BOARD_L1] & BOARD_FEATURE_BUTTON) << digitalPinToPCMSKbit(SWL)) |
    (!!(BOARD_FEATURES[BOARD_M] & BOARD_FEATURE_BUTTON) << digitalPinToPCMSKbit(SW1)) |
    (!!(BOARD_FEATURES[BOARD_R1] & BOARD_FEATURE_BUTTON) << digitalPinToPCMSKbit(SWR)));
}

inline void updateTouchStates() {
  for (byte i = 1; i < 4; ++i) { // Pads and buttons not available on leftmost and rightmost boards
    if (BOARD_FEATURES[i] & BOARD_FEATURE_TOUCH) {
      // TODO: Wut? What's this?
      touchStates |= TOUCH_PINS[i];
    }
  }
}

inline void updateEncoders() {
  ticks++;
#if HAS_FEATURE(L2, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_L2]).tick();
#endif
#if HAS_FEATURE(L1, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_L1]).tick();
#endif
#if HAS_FEATURE(M, BOARD_FEATURE_ENCODER)
#if !defined(USART_DEBUG_ENABLED) || PCB_VERSION != 1
  (*encoders[BOARD_M]).tick();
#endif
#endif
#if HAS_FEATURE(R1, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_R1]).tick();
#endif
#if HAS_FEATURE(R2, BOARD_FEATURE_ENCODER)
  (*encoders[BOARD_R2]).tick();
#endif
}

inline void updateControls() {
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_ENCODER)
  updateEncoders();
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
  updatePadStates();
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_TOUCH)
  updateTouchStates();
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
  updateSwitchStates();
#endif
}

ISR(PCINT0_vect) {
  updateControls();
    
#if defined(INTERRUPT_DEBUG) && (defined(USART_DEBUG_ENABLED) || defined(I2C_DEBUG_ENABLED))
  interrupter = 0;
#endif
}

ISR(PCINT1_vect) {
  updateControls();
  
#if defined(INTERRUPT_DEBUG) && (defined(USART_DEBUG_ENABLED) || defined(I2C_DEBUG_ENABLED))
  interrupter = 1;
#endif
}

ISR(PCINT2_vect) {
  updateControls();
  
#if defined(INTERRUPT_DEBUG) && (defined(USART_DEBUG_ENABLED) || defined(I2C_DEBUG_ENABLED))
  interrupter = 2;
#endif
}
