
#ifndef WEAS_UI_IFACE_H
#define WEAS_UI_IFACE_H

#include <cstdint>


class KnobChanges {

public:
  virtual void handle_knob_change(uint8_t /* knob */, uint16_t /* now */, uint16_t /* prev */) {};
  virtual ~KnobChanges() = default;

};


class ConnectedChanges {

public:
  virtual void handle_connected_change(uint8_t /* socket_in */, bool /* connected */) {};
  virtual ~ConnectedChanges() = default;

};


#endif
