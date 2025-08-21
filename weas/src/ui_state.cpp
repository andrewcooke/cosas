
#include "weas/ui_state.h"

#include "weas/codec.h"
#include "weas/debug.h"
#include "weas/leds_buffer.h"


UIState::UIState(App& app, Codec::SwitchPosition initial)
  : KnobChanges(), app(app) {
  handle_knob_change(Codec::Switch, initial, initial);
}


void UIState::handle_knob_change(uint8_t knob, uint16_t now, uint16_t prev) {
  if (knob_cleaner.append(knob, now, prev)) {
    now = knob_cleaner.get(knob, Now);
    prev = knob_cleaner.get(knob, Prev);
  } else return;
  if (state != prev_state) {
    Debug::log(prev_state, "->", state);
    prev_state = state;
  }
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
  case (SOURCE):
    state_source(knob, now, prev);
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
      saved_adjust_mask = buffer.get_mask();
      transition_to(current_source_mask());
      state = SOURCE;
      break;
    case (Codec::Down): {
      saved_adjust_mask = buffer.get_mask();
      transition_to(current_page_mask());
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

void UIState::transition_to(uint32_t mask) {
  LEDsBuffer &buffer = LEDsBuffer::get();
  uint32_t start = buffer.get_mask();
  buffer.queue(LEDsMask::vinterp(1,  start, mask), true, true, 0);
  buffer.queue(LEDsMask::vinterp(2, start, mask), false, true, 0);
  buffer.queue(mask, false, true, 0);
}

uint32_t UIState::current_page_mask() {
  // +1 is shifting from 0 to 1 index
  return LEDsMask::rot2dot(page + 1, LEDsMask::BITS_MASK, LEDsMask::BITS_MASK >> 2);
}

void UIState::state_next_page(uint8_t knob, uint16_t now, uint16_t) {
  LEDsBuffer &buffer = LEDsBuffer::get();
  switch (knob) {
  case (Codec::Switch):
    switch (now) {
    case (Codec::Middle):
      page = (page + 1) % app.get_n_pages(source);
      buffer.queue(current_page_mask(), true, true, LEDsBuffer::INTERP_N << 1);
      transition_to(saved_adjust_mask);
      state = ADJUST;
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
      transition_to(saved_adjust_mask);
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

void UIState::state_source(uint8_t knob, uint16_t now, uint16_t prev) {
  LEDsBuffer &buffer = LEDsBuffer::get();
  switch (knob) {
  case (Codec::Switch):
    switch (now) {
    case (Codec::Middle):
      transition_to(saved_adjust_mask);
      state = ADJUST;
      break;
    default:
      break;
    }
    break;
  case (Codec::Main): {
    KnobChange change = source_knob.handle_knob_change(now, prev);
    source = static_cast<uint>(app.get_n_sources() * change.normalized);
    buffer.queue(current_source_mask(), true, false, 0);
    break;
  }
  default:
    break;
  }
}

uint32_t UIState::current_source_mask() {
  // +1 is shifting from 0 to 1 index
  return LEDsMask::rot2dot(source + 1, LEDsMask::BITS_MASK >> 3, LEDsMask::BITS_MASK >> 1);
}

