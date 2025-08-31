
#include "weas/ui_state.h"

#include "weas/leds_buffer.h"
#include "weas/codec.h"
#include "weas/debug.h"


UIState::UIState(App& app, FIFO& fifo,  CtrlEvent::SwitchPosition initial)
  : CtrlHandler(), app(app), fifo(fifo), buffer(LEDsBuffer::get()),
    leds_mask(buffer.leds_mask.get()) {
  update_source();
  handle_ctrl_change(CtrlEvent(CtrlEvent::Switch, initial, initial));
}


void UIState::handle_ctrl_change(CtrlEvent event) {
  // static uint processed = 0;
  if (knob_damper.append(event)) {
    // if (!(processed++ & ((1 << 4) - 1))) Debug::log("processed", processed);
    event = knob_damper.get();
    switch (state) {
    case (ADJUST):
      state_adjust(event);
      break;
    case (NEXT_PAGE):
      state_next_page(event);
      break;
    case (FREEWHEEL):
      state_freewheel(event);
      break;
    case (SOURCE):
      state_source(event);
    default:
      break;
    }
  }
}

void UIState::state_adjust(CtrlEvent event) {
  switch (event.ctrl) {
  case (CtrlEvent::Switch): {
    switch (event.now) {
    case (CtrlEvent::Up):
      saved_adjust_mask = buffer.get_mask();
      saved_source_idx = source_idx;
      transition_leds_to(current_source_mask(), false);
      state = SOURCE;
      break;
    case (CtrlEvent::Down): {
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
  case (CtrlEvent::Main):
  case (CtrlEvent::X):
  case (CtrlEvent::Y): {
    if (current_page_knobs[event.ctrl]->is_valid()) {
      auto stalled = Stalled(fifo);
      KnobChange change = current_page_knobs[event.ctrl]->handle_knob_change(event.now, event.prev);
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
  // +1 is shifting from 0 to 1 indx
  return leds_mask->rot2dot(page + 1, leds_mask->BITS_MASK, leds_mask->BITS_MASK >> 2);
}

void UIState::state_next_page(CtrlEvent event) {
  switch (event.ctrl) {
  case (CtrlEvent::Switch):
    switch (event.now) {
    case (CtrlEvent::Middle):
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
  case (CtrlEvent::Main):
  case (CtrlEvent::X):
  case (CtrlEvent::Y):
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
    current_page_knobs[i] = std::make_unique<ParamAdapter>(app.get_param(page, static_cast<Knob>(i)));
    // current_page_knobs[i] = std::make_unique<ParamAdapter>(*param);
  }
}

void UIState::state_freewheel(CtrlEvent event) {
  switch (event.ctrl) {
  case (CtrlEvent::Switch):
    switch (event.now) {
    case (CtrlEvent::Middle): {
      transition_leds_to(saved_adjust_mask, false);
      state = ADJUST;
      break;
    }
    default:
      break;
    }
  case (CtrlEvent::Main):
  case (CtrlEvent::X):
  case (CtrlEvent::Y):
  default:
    break;
  }
}

void UIState::state_source(CtrlEvent event) {
  switch (event.ctrl) {
  case (CtrlEvent::Switch):
    switch (event.now) {
    case (CtrlEvent::Middle):
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
  case (CtrlEvent::Main): {
    KnobChange change = source_knob.handle_knob_change(event.now, event.prev);
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

