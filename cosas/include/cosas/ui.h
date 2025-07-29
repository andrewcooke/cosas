
#ifndef COSAS_UI_H
#define COSAS_UI_H

#include <cstdint>


class KnobChanges {

public:
  virtual void handle_knob_change(uint8_t /* knob */, uint16_t /* now */, uint16_t /* prev */) {};
  virtual ~KnobChanges() = default;

};

static KnobChanges default_knob_changes;


#endif
