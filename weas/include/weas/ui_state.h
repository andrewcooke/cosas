
#ifndef WEAS_UI_STATE_H
#define WEAS_UI_STATE_H


#include "cosas/app.h"
#include "cosas/filter.h"
#include "cosas/knobs.h"

#include "weas/codec.h"
#include "weas/leds_direct.h"
#include "weas/leds_buffer.h"


class UIState final : public KnobChanges {

public:

  UIState(App& app, Codec::SwitchPosition initial);
  void handle_knob_change(uint8_t knob, uint16_t now, uint16_t prev) override;

private:

  App& app;
  LEDsBuffer& buffer;
  BaseLEDsMask* leds_mask;
  enum State {ADJUST, NEXT_PAGE, FREEWHEEL, SOURCE};
  State state = SOURCE;
  State prev_state = SOURCE;
  uint page = 0;
  uint source = 0;
  static constexpr uint8_t amp = 0x6;
  LEDsDirect leds;
  std::array<std::unique_ptr<Knob>, Codec::N_KNOBS - 1> knobs = {
    std::make_unique<Knob>(0.5, 0, true, 0, 3),
    std::make_unique<Knob>(0.5, 1, false, 0, 1),
    std::make_unique<Knob>()};
  KnobCleaner knob_cleaner = KnobCleaner(1, 16);
  Knob source_knob = Knob(1, 1, false, 0, 1);

  void state_adjust(uint8_t knob, uint16_t now, uint16_t prev);
  void state_next_page(uint8_t knob, uint16_t now, uint16_t prev);
  void state_freewheel(uint8_t knob, uint16_t now, uint16_t prev);
  void state_source(uint8_t knob, uint16_t now, uint16_t prev);

  uint32_t current_page_mask();
  uint32_t current_source_mask();
  void transition_to(uint32_t mask, bool down);
  uint32_t saved_adjust_mask = 0;

};


#endif

