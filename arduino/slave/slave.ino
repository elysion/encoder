#include <stdint.h>

#include "config.h" // these imports need to be in this order because slave.h uses defs in config.h
#include "slave.h"

#ifdef BOARD_HAS_DEBUG_LED
#include <MillisTimer.h>
#endif

#include "feature_validation.h"

Board firstBoardInLedChain(Board board) {
  return (Board) (board - (board % 2));
}

#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_LED)
inline uint32_t colorForPosition(uint8_t position) {
  return position % 4 == 0 ? Slave.ColorHSV(0, UINT8_MAX, 20) :
         position % 4 == 1 ? Slave.ColorHSV(UINT16_MAX / 4, UINT8_MAX, 20) :
         position % 4 == 2 ? Slave.ColorHSV(UINT16_MAX / 4 * 2, UINT8_MAX, 20) :
                             Slave.ColorHSV(UINT16_MAX / 4 * 3, UINT8_MAX, 20);
}
#endif

inline void setPositionLedOn(uint8_t position, Board board) {
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_LED)
  const uint8_t firstIndex = board % 2 == 0 ? 0 : LED_COUNTS[board - 1];
  Slave.setLedColor(position + firstIndex, colorForPosition(position));
#endif
}

void setLedPosition(Board board, byte position) {
#if ANY_BOARD_HAS_FEATURE(BOARD_FEATURE_LED)
  Slave.initializeLedsForBoard(board);
  Slave.fillLeds(Slave.Color(0, 0, 0), 0, Slave.ledCountForChain(board));
  const Board firstBoard = firstBoardInLedChain(board);
  setPositionLedOn(Slave.getPosition(firstBoard), firstBoard);
  setPositionLedOn(Slave.getPosition(firstBoard+1), firstBoard+1);
  Slave.showLeds();
#endif
}

void sendChangeMessage(byte input, uint16_t value, ControlType type) {
  Slave.toggleBuiltinLed();
  Slave.sendMessageToMaster(input, value, type);
}

void handleChange(Board board, ControlType type, uint8_t input, uint8_t state) {
  switch (type) {
    // TODO: prevent input collisions on different boards
    case CONTROL_TYPE_POSITION: {
      #ifdef USART_DEBUG_ENABLED
        Serial.print("Pos change, board: ");
        Serial.print(board);
        Serial.print(", input: ");
        Serial.print(input);
        Serial.print(", pos: ");
        Serial.println(state);
      #endif
      sendChangeMessage(board, state, type);
      setLedPosition(board, state);

      break;
    }
    case CONTROL_TYPE_ENCODER: {
      #ifdef USART_DEBUG_ENABLED
        Serial.print("handleEncoderChange, input: ");
        Serial.print(input);
        Serial.print(", state: ");
        Serial.println(state);
      #endif
      sendChangeMessage(board, state, type);
      break;
    }
    case CONTROL_TYPE_BUTTON: {
//      handleButtonChange(boardMatrixIndex * MATRIX_INPUTS * MATRIX_OUTPUTS + MATRIX_INPUTS * output + input, currentState);
//      handleButtonChange(i, (switchStates & switchMask) ? 0 : 1);
//      handleButtonChange(i, pinState);
      sendChangeMessage(board * 20 + input, state, type);
    }
    default:
      sendChangeMessage(board, state, type);
  }
}

#ifdef BOARD_HAS_DEBUG_LED
void blinkBuiltinLed(MillisTimer &timer __attribute__((unused))) {
  Slave.toggleBuiltinLed();
}
MillisTimer blinkTimer = MillisTimer(1000, blinkBuiltinLed);
#endif

void setup() {
  #ifdef USART_DEBUG_ENABLED
  Serial.begin(115200);
  #endif
  Slave.setup(handleChange);
  #ifdef BOARD_HAS_DEBUG_LED
  blinkTimer.start();
  #endif

  for (byte i = 0; i <= BOARD_R2; ++i) {
    setLedPosition(i, 0);
  }
}

void loop() {
  Slave.update();

  #ifdef BOARD_HAS_DEBUG_LED
  blinkTimer.run();
  #endif
}
