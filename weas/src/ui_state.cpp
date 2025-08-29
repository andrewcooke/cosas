
#include "weas/ui_state.h"

#include "weas/leds_buffer.h"
#include "weas/codec.h"
#include "weas/debug.h"


UIState::UIState(App& app, FIFO& fifo,  Codec::SwitchPosition initial)
  : CtrlChanges(), app(app), fifo(fifo), buffer(LEDsBuffer::get()),
    leds_mask(buffer.leds_mask.get()) {
  update_source();
  handle_ctrl_change(Codec::Switch, initial, initial);
}


void UIState::handle_ctrl_change(uint8_t knob, uint16_t now, uint16_t prev) {
  // static uint processed = 0;
  if (knob_damper.append(knob, now, prev)) {
    // if (!(processed++ & ((1 << 4) - 1))) Debug::log("processed", processed);
    now = knob_damper.get(knob, Now);
    prev = knob_damper.get(knob, Prev);
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
}

void UIState::state_adjust(uint8_t knob, uint16_t now, uint16_t prev) {
  switch (knob) {
  case (Codec::Switch): {
    switch (now) {
    case (Codec::Up):
      saved_adjust_mask = buffer.get_mask();
      saved_source_idx = source_idx;
      transition_leds_to(current_source_mask(), false);
      state = SOURCE;
      break;
    case (Codec::Down): {
      saved_adjust_mask = buffer.get_mask();
      transition_leds_to(current_page_mask(), true);
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
    if (current_page_knobs[knob]->is_valid()) {
      auto stalled = Stalled(fifo);
      KnobChange change = current_page_knobs[knob]->handle_knob_change(now, prev);
      uint32_t ring = leds_mask->ring(change.normalized, change.highlight);
      buffer.queue(ring, false, false, 0);
    } else {
      buffer.queue(INVALID_KNOB, false, false, 0);
    }
    break;
  }
  default:
    break;
  }
}

void UIState::transition_leds_to(uint32_t mask, bool down) {
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
      // changing page!
      page = (page + 1) % app.n_pages();
      buffer.queue(current_page_mask(), false, false, buffer.INTERP_N << 2);  // keep it there a while
      transition_leds_to(saved_adjust_mask, false);
      update_page();
      state = ADJUST;
      break;
    default:
      break;
    }
    break;
  case (Codec::Main):
  case (Codec::X):
  case (Codec::Y):
    state = FREEWHEEL;
    break;
  default:
    break;
  }
}

void UIState::update_source() {
  // TODO - connect the source
  source = app.get_source(source_idx);
  page = 0;
  update_page();
}

void UIState::update_page() {
  for (uint i = 0; i < N_KNOBS; i++) {
    current_page_knobs[i] = std::make_unique<ParamHandler>(app.get_param(page, static_cast<Knob>(i)));
  }
}

void UIState::state_freewheel(uint8_t knob, uint16_t now, uint16_t /* prev */) {
  switch (knob) {
  case (Codec::Switch):
    switch (now) {
    case (Codec::Middle): {
      transition_leds_to(saved_adjust_mask, false);
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
      transition_leds_to(saved_adjust_mask, true);
      state = ADJUST;
      if (source_idx != saved_source_idx) {
        // we have selected a new source
        update_source();
      }
      break;
    default:
      break;
    }
    break;
  case (Codec::Main): {
    KnobChange change = source_knob.handle_knob_change(now, prev);
    source_idx = static_cast<uint>(app.n_sources() * change.normalized);
    buffer.queue(current_source_mask(), false, false, 0);
    break;
  }
  default:
    break;
  }
}

uint32_t UIState::current_source_mask() {
  // +1 is shifting from 0 to 1 index
  return leds_mask->wiggle19(source_idx, leds_mask->BITS_MASK >> 1, leds_mask->BITS_MASK >> 3);
}

