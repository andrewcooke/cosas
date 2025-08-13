
#ifndef WEAS_UI_STATE_H
#define WEAS_UI_STATE_H


#include "weas/codec.h"
#include "weas/knobs.h"
#include "weas/leds_direct.h"
#include "weas/leds_mask.h"
#include "weas/leds_timer.h"


class UIState : public KnobChanges {

public:

  explicit UIState(Codec& codec);
  void handle_knob_change(uint8_t knob, uint16_t now, uint16_t prev) override;

private:

  enum State {ADJUST, FREEWHEEL, META};
  State state = ADJUST;
  static constexpr uint8_t amp = 0x6;
  LEDsDirect leds;
  LEDsMask leds_mask;
  LEDsTimer& leds_timer;
  // afaict the "3" should not be needed here, but i get an error without it.
  uint32_t overlay[Codec::N_KNOBS -1] = {
    leds_mask.square(0, amp),
    leds_mask.vbar(0, amp),
    leds_mask.square(1, amp)};
  Knob knobs[Codec::N_KNOBS -1] = {Knob(), Knob(), Knob()};

  void state_adjust(uint8_t k, uint16_t now, uint16_t prev);
  void state_freewheel(uint8_t knob, uint16_t now, uint16_t prev);

};



#endif

