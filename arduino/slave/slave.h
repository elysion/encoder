#pragma once

typedef uint8_t byte;
#include <pins_arduino.h>

#include "features.h"
#include "shared.h"
#include "types.h"
// TODO: if this is not imported here, the initialization will fail and the device will not work properly
// TODO: this should be fixed in order to be able to use this code as a library
#include "config.h"

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_LED)
#include <Adafruit_NeoPixel.h>
#endif
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_ENCODER)
#include <RotaryEncoder.h>  
#endif

struct ButtonPairStates {
  bool firstButtonState;
  bool secondButtonState;
};

static const uint8_t ENCODER_PINS[BOARD_COUNT][2] = {
  {ENCL1A, ENCL1B},
  {ENCL2A, ENCL2B},
#if PCB_VERSION == 3
  {ENCM1A, ENCM1B},
  {ENCM2A, ENCM2B},
#else
  {ENC1A, ENC1B},
#endif
  {ENCR1A, ENCR1B},
  {ENCR2A, ENCR2B}
};

class Slave_;
typedef void (*ChangeHandler)(Board, ControlType, uint8_t /*input*/, uint8_t /*state*/);

class Slave_
{
public:
  Slave_();
  void setup(ChangeHandler = NULL);
  void update();
  void sendMessageToMaster(byte input, uint16_t value, ControlType type);
  void toggleBuiltinLed();
  void tickEncoder(Board board);
  int getPosition(Board board);
  uint8_t ledCountForChain(Board board);
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
  void updateSwitchStates();
#endif
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
  void updatePadStates();
#endif
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_LED)
  void initializeLedsForBoard(Board board);
  void setLedColor(uint16_t position, uint32_t color);
  void fillLeds(uint32_t color, uint16_t first = 0, uint16_t count = 0);
  void showLeds();

  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    return Adafruit_NeoPixel::Color(r, g, b);
  }
  static uint32_t ColorHSV(uint16_t h, uint8_t s, uint8_t v) {
    return Adafruit_NeoPixel::ColorHSV(h, s, v);
  }
#endif

private:
  inline void setupI2c();
  inline void setupPinModes();
  inline void setupInterrupts();
  inline uint8_t readPadPin(uint8_t board, uint8_t pin);
  void handleButtonChange(uint8_t input, uint8_t state); // TODO make this customizable
  void handlePositionChange(uint8_t input, uint8_t state); // TODO make this customizable
  uint8_t requestAddress();
  void sendMessageToMaster(SlaveToMasterMessage& message);

  uint8_t ledCountForBoard(Board board);
  uint8_t ledPinForBoard(Board board);
  uint8_t firstLedIndex(Board board);

  #if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
  ButtonPairStates voltageToButtonStates(int voltage);
  uint8_t getButtonStates();
  #endif

  ChangeHandler handler;

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_LED)
  uint8_t currentLedPin = 0;
  Adafruit_NeoPixel *leds = 0;
#endif

  volatile uint8_t address;

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
  uint8_t switchStates;
  uint8_t previousSwitchStates;
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_TOUCH)
#if PCB_VERSION != 3 // TODO
  uint8_t touchStates;
  uint8_t previousTouchStates;
#endif
#endif

// TODO: config should somehow be project specific if this code is to be used as a library
  #if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_MATRIX)
  uint8_t previousMatrixButtonStates[MAX_MATRIX_BOARD_COUNT][MATRIX_OUTPUTS] = {
    {
      0,
      0,
      0
    },
    {
      0,
      0,
      0
    }
  };
  #endif

  #if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
  uint8_t padStates[3];
  uint8_t previousPadStates[3];
  #endif

  #if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_ENCODER)
  RotaryEncoder* encoders[BOARD_COUNT] = {
  #if BOARD_FEATURES_L1 & BOARD_FEATURE_ENCODER
  new RotaryEncoder(ENCODER_PINS[BOARD_L1][0], ENCODER_PINS[BOARD_L1][1])
  #else
  0
  #endif
  ,
  #if BOARD_FEATURES_L2 & BOARD_FEATURE_ENCODER
  new RotaryEncoder(ENCODER_PINS[BOARD_L2][0], ENCODER_PINS[BOARD_L2][1])
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

  int positions[BOARD_COUNT]  = {
    0,
    0,
    0,
  #if PCB_VERSION == 3
    0,
  #endif
    0,
    0
  };
  #endif

  #ifdef ENCODER_PIN_DEBUG_ENABLED
  uint8_t states[BOARD_COUNT*2] = {
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
};

#define BOARD_MATRIX_INDEX(BOARD) (BOARD == BOARD_L1 ? 0 : BOARD == BOARD_R1 ? 1 : -1)

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_BUTTON)
const int FIRST_BUTTON_VOLTAGE = 485;
const int SECOND_BUTTON_VOLTAGE = 855;
const int BOTH_BUTTONS_VOLTAGE = 583;
const int BUTTON_VOLTAGE_RANGE = min(BOTH_BUTTONS_VOLTAGE - FIRST_BUTTON_VOLTAGE, SECOND_BUTTON_VOLTAGE - BOTH_BUTTONS_VOLTAGE) / 2;

#if PCB_VERSION == 3
static const uint8_t BUTTON_PINS[] = {
  SWL,
  SWL,
  SWM,
  SWM,
  SWR,
  SWR
};
#else
static const uint8_t BUTTON_PINS[] = {
  SWL,
  NOT_POSSIBLE,
  SWM,
  SWR
};
#endif
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_POT)
static const uint8_t POT_CHANGE_THRESHOLD = 5;

#if PCB_VERSION != 3
static const uint8_t POT_PINS[] = {
  POTL,
  NOT_POSSIBLE,
  POTM,
  POTR
};
#endif

#if PCB_VERSION != 3
static const uint8_t TOUCH_PINS[] = {
  ENCL2B,
  NOT_POSSIBLE,
  TOUCH,
  ENCR2B
};
#endif
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_PADS)
#if PCB_VERSION != 3
static const uint8_t PAD_PINS[][4] {
  {ENCL2A, ENCL1B, ENCL1A, SWL},
  {},
  {TOUCH, ENC1B, ENC1A, SWM}, // TODO: check version 3
  {ENCR1B, ENCR1A, ENCR2A, SWR}
};
#endif
#endif

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_MATRIX)
// TODO: create a button matrix with LED support?
static const uint8_t MATRIX_OUTPUTS = 3;
static const uint8_t MATRIX_INPUTS = 3;
static const uint8_t MAX_MATRIX_BOARD_COUNT = 2;

static const uint8_t BUTTON_MATRIX_INPUT_PINS[MAX_MATRIX_BOARD_COUNT][MATRIX_INPUTS] = {
  {
    ENCL2B,
    ENCL2A,
    SWL
  },
  {
    ENCR1B,
    ENCR1A,
#if PCB_VERSION == 3
    LEDR // TODO: check that this is the correct pin
#else
    LED2
#endif
  }
};

static const uint8_t BUTTON_MATRIX_OUTPUT_PINS[MAX_MATRIX_BOARD_COUNT][MATRIX_OUTPUTS] = {
  {
    ENCL1B,
    ENCL1A,
#if PCB_VERSION == 3
    LEDL // TODO: check that this is the correct pin
#else
    LED1 // TODO: Which pin should this be? Jumper missing on board?
#endif
  },
  {
    ENCR2B,
    ENCR2A,
    SWR
  }
};
#endif

#if PCB_VERSION == 3
#define BOARD_HAS_DEBUG_LED
#endif

extern Slave_ Slave;
