
#ifndef WEAS_UI_STATE_H
#define WEAS_UI_STATE_H


#include "cosas/app.h"

#include "weas/codec.h"
#include "weas/knobs.h"
#include "weas/leds_buffer.h"
#include "weas/leds_direct.h"
#include "weas/leds_mask.h"


class UIState : public KnobChanges {

public:

  UIState(App& app) : KnobChanges(), app(app) {}
  void handle_knob_change(uint8_t knob, uint16_t now, uint16_t prev) override;

private:

  App& app;
  enum State {ADJUST, NEXT_PAGE, FREEWHEEL, SELECT};
  State state = ADJUST;
  uint page = 0;
  uint n_pages = 6;
  static constexpr uint8_t amp = 0x6;
  LEDsDirect leds;
  std::array<std::unique_ptr<Knob>, Codec::N_KNOBS - 1> knobs = {
    std::make_unique<Knob>(0.5, 0, true, 0, 3),
    std::make_unique<Knob>(0.5, 1, false, 0, 1),
    std::make_unique<Knob>()};

  void state_adjust(uint8_t knob, uint16_t now, uint16_t prev);
  void state_next_page(uint8_t knob, uint16_t now, uint16_t prev);
  void state_freewheel(uint8_t knob, uint16_t now, uint16_t prev);
  void state_select(uint8_t knob, uint16_t now, uint16_t prev);

};


#endif

