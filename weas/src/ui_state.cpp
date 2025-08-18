
#include "weas/ui_state.h"
#include "weas/leds_buffer.h"

void UIState::handle_knob_change(uint8_t knob, uint16_t now, uint16_t prev) {
  switch (state) {
  case (ADJUST):
    state_adjust(knob, now, prev);
    break;
  case (NEXT_PAGE):
    state_next_page(knob, now, prev);
    break;
  case (FREEWHEEL):
    state_freewheel(knob, now, prev);
    break;
  case (SELECT):
    state_select(knob, now, prev);
  default:
    break;
  }
}

void UIState::state_adjust(uint8_t knob, uint16_t now, uint16_t prev) {
  LEDsBuffer &buffer = LEDsBuffer::get();
  switch (knob) {
  case (Codec::Switch): {
    switch (now) {
    case (Codec::Up):
      state = SELECT;
      break;
    case (Codec::Down): {
      uint32_t m = LEDsBuffer::get().get_mask();
      buffer.queue(LEDsMask::vinterp(1, m, m), true, false, 1);
      state = NEXT_PAGE;
      break;
    }
    default:
      break;
    }
    break;
  }
  case (Codec::Main):
  case (Codec::X):
  case (Codec::Y): {
    KnobChange change = knobs[knob]->handle_knob_change(now, prev);
    uint32_t ring = LEDsMask::ring(change.normalized, change.highlight);
    buffer.queue(ring, false, false, 0);
    break;
  }
  default:
    break;
  }
}

void UIState::state_next_page(uint8_t knob, uint16_t now, uint16_t) {
  LEDsBuffer &buffer = LEDsBuffer::get();
  switch (knob) {
  case (Codec::Switch):
    switch (now) {
    case (Codec::Middle):
      page = (page + 1) % n_pages;
      buffer.queue(LEDsMask::rot2dot(page, LEDsMask::BITS_MASK,
                                     LEDsMask::BITS_MASK >> 2),
                   true, true, LEDsBuffer::INTERP_N << 1);
      state = ADJUST;
      sleep_ms(500); // so visible despite noise
      break;
    default:
      break;
    }
    break;
  case (Codec::Main):
  case (Codec::X):
  case (Codec::Y):
    // TODO - may need to accumulate and threshold?
    state = FREEWHEEL;
    break;
  default:
    break;
  }
}

void UIState::state_freewheel(uint8_t knob, uint16_t now, uint16_t /* prev */) {
  // LEDsBuffer &buffer = LEDsBuffer::get();
  switch (knob) {
  case (Codec::Switch):
    switch (now) {
    case (Codec::Middle): {
      state = ADJUST;
      break;
    }
    default:
      break;
    }
  case (Codec::Main):
  case (Codec::X):
  case (Codec::Y):
    break;
  default:
    break;
  }
}

void UIState::state_select(uint8_t knob, uint16_t now, uint16_t) {
  // LEDsBuffer &buffer = LEDsBuffer::get();
  switch (knob) {
  case (Codec::Switch):
    switch (now) {
    case (Codec::Middle):
      state = ADJUST;
      break;
    default:
      break;
    }
  case (Codec::Main):
    break;
  default:
    break;
  }
}
