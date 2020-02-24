#ifndef SHARED_H
#define SHARED_H

enum ControlType {
  CONTROL_TYPE_DEBUG,
  CONTROL_TYPE_ENCODER,
  CONTROL_TYPE_BUTTON,
  CONTROL_TYPE_POSITION,
  CONTROL_TYPE_TOUCH,
  CONTROL_TYPE_DATA
};

enum DebugMessage {
  DEBUG_BOOT,
  DEBUG_RECEIVED_ADDRESS
};

const uint8_t SlaveToMasterMessageSize = 5;
struct SlaveToMasterMessage {
  uint8_t address;
  uint8_t input;
  ControlType type;
  uint16_t value;
};

#endif // SHARED_H