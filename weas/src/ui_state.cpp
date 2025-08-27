
#include "weas/ui_state.h"

#include "weas/leds_buffer.h"
#include "weas/codec.h"
#include "weas/debug.h"


UIState::UIState(App& app, Codec::SwitchPosition initial)
  : CtrlChanges(), app(app), buffer(LEDsBuffer::get()), leds_mask(buffer.leds_mask.get()) {
  handle_ctrl_change(Codec::Switch, initial, initial);
  // TODO - here too
  app.get_source(source);

}


void UIState::handle_ctrl_change(uint8_t knob, uint16_t now, uint16_t prev) {
  if (knob_cleaner.append(knob, now, prev)) {
    now = knob_cleaner.get(knob, Now);
    prev = knob_cleaner.get(knob, Prev);
  } else return;
  if (state != prev_state) {
    // Debug::log(prev_state, "->", state);
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
  switch (knob) {
  case (Codec::Switch): {
    switch (now) {
    case (Codec::Up):
      saved_adjust_mask = buffer.get_mask();
      transition_to(current_source_mask(), false);
      state = SOURCE;
      break;
    case (Codec::Down): {
      saved_adjust_mask = buffer.get_mask();
      transition_to(current_page_mask(), true);
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
    uint32_t ring = leds_mask->ring(change.normalized, change.highlight);
    buffer.queue(ring, false, false, 0);
    break;
  }
  default:
    break;
  }
}

void UIState::transition_to(uint32_t mask, bool down) {
  uint32_t start = buffer.get_mask();
  if (down) {
    buffer.queue(leds_mask->vinterp(1, start, mask), false, true, 0);
    buffer.queue(leds_mask->vinterp(2, start, mask), false, true, 0);
  } else {
    buffer.queue(leds_mask->vinterp(2, mask, start), false, true, 0);
    buffer.queue(leds_mask->vinterp(1, mask, start), false, true, 0);
  }
  buffer.queue(mask, false, true, 0);
}

uint32_t UIState::current_page_mask() {
  // +1 is shifting from 0 to 1 index
  return leds_mask->rot2dot(page + 1, leds_mask->BITS_MASK, leds_mask->BITS_MASK >> 2);
}

void UIState::state_next_page(uint8_t knob, uint16_t now, uint16_t) {
  switch (knob) {
  case (Codec::Switch):
    switch (now) {
    case (Codec::Middle):
      page = (page + 1) % app.n_pages();
      buffer.queue(current_page_mask(), false, false, buffer.INTERP_N << 2);  // keep it there a while
      transition_to(saved_adjust_mask, false);
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
  switch (knob) {
  case (Codec::Switch):
    switch (now) {
    case (Codec::Middle): {
      transition_to(saved_adjust_mask, false);
      state = ADJUST;
      break;
    }
    default:
      break;
    }
  case (Codec::Main):
  case (Codec::X):
  case (Codec::Y):
  default:
    break;
  }
}

void UIState::state_source(uint8_t knob, uint16_t now, uint16_t prev) {
  switch (knob) {
  case (Codec::Switch):
    switch (now) {
    case (Codec::Middle):
      transition_to(saved_adjust_mask, true);
      state = ADJUST;
      break;
    default:
      break;
    }
    break;
  case (Codec::Main): {
    KnobChange change = source_knob.handle_knob_change(now, prev);
    source = static_cast<uint>(app.n_sources() * change.normalized);
    // TODO - use the source
    app.get_source(source);
    buffer.queue(current_source_mask(), false, false, 0);
    break;
  }
  default:
    break;
  }
}

uint32_t UIState::current_source_mask() {
  // +1 is shifting from 0 to 1 index
  return leds_mask->wiggle19(source, leds_mask->BITS_MASK >> 1, leds_mask->BITS_MASK >> 3);
}

