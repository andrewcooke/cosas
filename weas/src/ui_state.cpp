
#include "weas/ui_state.h"


UIState::UIState(Codec& codec) : leds_timer(LEDsTimer::get(codec)) {};

void UIState::handle_knob_change(uint8_t knob, uint16_t now, uint16_t prev) {
  switch (state) {
  case (ADJUST):
    state_adjust(knob, now, prev);
    break;
  case (FREEWHEEL):
    state_freewheel(knob, now, prev);
    break;
  default:
    break;
  }
}

void UIState::state_adjust(uint8_t k, uint16_t now, uint16_t prev) {
  switch (k) {
  case (Codec::Main):
  case (Codec::X):
  case (Codec::Y): {
    KnobChange change = knobs[k]->handle_knob_change(now, prev);
    uint32_t ring = leds_mask.ring(change.normalized, change.highlight);
    leds_timer.show(ring, overlay[k]);
    break;
  }
  case (Codec::Switch): {
    if (now == Codec::Down) {
      uint32_t m = leds_timer.get_mask();
      uint32_t e = leds_timer.get_extra();
      leds_timer.show(leds_mask.vinterp(1, m, m), leds_mask.vinterp(1, e, e));
      state = FREEWHEEL;
    } else if (now == Codec::Up) {
      leds_timer.clear_loop();
      // TODO
    }
    break;
  }
  default:
    break;
  }
}

void UIState::state_freewheel(uint8_t knob, uint16_t now, uint16_t /* prev */) {
  switch (knob) {
  case (Codec::Switch):
    if (now == Codec::Middle) state = ADJUST;
    break;
  default:
    break;
  }
}
