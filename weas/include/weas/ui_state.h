
#ifndef WEAS_UI_STATE_H
#define WEAS_UI_STATE_H


#include "cosas/app.h"
#include "cosas/filter.h"
#include "cosas/knobs.h"

#include "weas/codec.h"
#include "weas/leds_buffer.h"


class UIState final : public CtrlChanges {

public:

  UIState(App& app, Codec::SwitchPosition initial);
  void handle_ctrl_change(uint8_t knob, uint16_t now, uint16_t prev) override;

private:

  App& app;
  LEDsBuffer& buffer;
  BaseLEDsMask* leds_mask;
  enum State {ADJUST, NEXT_PAGE, FREEWHEEL, SOURCE};
  State state = SOURCE;  // start in meta so when we transition knobs are set
  State prev_state = SOURCE;
  uint32_t INVALID_KNOB = leds_mask->constant(0x08);
  uint page = 0;
  uint source_idx = 0;
  uint saved_source_idx = 0;  // used to check if we changed source
  RelSource* source = nullptr;
  std::array<std::unique_ptr<KnobHandler>, Codec::N_CTRLS - 1> current_page_knobs = {
    std::make_unique<KnobHandler>(),
    std::make_unique<KnobHandler>(),
    std::make_unique<KnobHandler>()};
  CtrlDamper knob_damper = CtrlDamper(1, 64);
  KnobHandler source_knob = KnobHandler(1, 1, false, 0, 1);

  void state_adjust(uint8_t knob, uint16_t now, uint16_t prev);
  void state_next_page(uint8_t knob, uint16_t now, uint16_t prev);
  void state_freewheel(uint8_t knob, uint16_t now, uint16_t prev);
  void state_source(uint8_t knob, uint16_t now, uint16_t prev);

  uint32_t current_page_mask();
  uint32_t current_source_mask();
  void transition_leds_to(uint32_t mask, bool down);
  void update_source();
  void update_page();
  uint32_t saved_adjust_mask = 0;

};


#endif

