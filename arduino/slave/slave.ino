#include "config.h" // these imports need to be in this order because slave.h uses defs in config.h
#include "slave.h"

#ifdef BOARD_HAS_DEBUG_LED
#include <MillisTimer.h>
#endif

#include "feature_validation.h"

void setLedPosition(Board board, byte position) {
  uint32_t color = board < 2 ? Slave.Color(20, 0, 0) : board > 3 ? Slave.Color(0, 0, 20) : Slave.Color(0, 20, 0);
  board = BOARD_R1;
  Slave.fillLeds(board, Slave.Color(1, 1, 1));
  Slave.setLedColor(board, position, color);
  Slave.showLeds(board);
}

void handleChange(Board board, ControlType type, uint8_t input, uint8_t state) {
  switch (type) {
    // TODO: prevent input collisions on different boards
    case CONTROL_TYPE_POSITION: {
      #if defined(USART_DEBUG_ENABLED)
        Serial.print("Pos change, board: ");
        Serial.print(board);
        Serial.print(", input: ");
        Serial.print(input);
        Serial.print(", pos: ");
        Serial.println(state);
      #endif
      Slave.sendMessage(board, state, type);
      setLedPosition(board, state);

      break;
    }
    case CONTROL_TYPE_ENCODER: {
      #if defined(USART_DEBUG_ENABLED)
        Serial.print("handleEncoderChange, input: ");
        Serial.print(input);
        Serial.print(", state: ");
        Serial.println(state);
      #endif
      Slave.sendMessage(board, state, type);
      break;
    }
    case CONTROL_TYPE_BUTTON: {
//      handleButtonChange(boardMatrixIndex * MATRIX_INPUTS * MATRIX_OUTPUTS + MATRIX_INPUTS * output + input, currentState);
//      handleButtonChange(i, (switchStates & switchMask) ? 0 : 1);
//      handleButtonChange(i, pinState);
      Slave.sendMessage(board, state, type);
    }
    default:
      Slave.sendMessage(board, state, type);
  }
}

#ifdef BOARD_HAS_DEBUG_LED
void blinkBuiltinLed(MillisTimer &timer __attribute__((unused))) {
  Slave.toggleBuiltinLed();
}
MillisTimer blinkTimer = MillisTimer(1000, blinkBuiltinLed);
#endif

void setup() {
  Serial.begin(115200);
  Slave.setup(handleChange);
  #ifdef BOARD_HAS_DEBUG_LED
  blinkTimer.start();
  #endif
}

#ifdef PORT_STATE_DEBUG
byte previousB = 0;
byte previousC = 0;
byte previousD = 0;
#endif

void loop() {
  Slave.update();
  
  #ifdef BOARD_HAS_DEBUG_LED
  blinkTimer.run();
  #endif
}
