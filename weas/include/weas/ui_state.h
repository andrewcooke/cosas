
#ifndef WEAS_UI_STATE_H
#define WEAS_UI_STATE_H


#include "cosas/app.h"
#include "cosas/filter.h"
#include "cosas/knobs.h"
#include "cosas/patomic.h"

#include "weas/codec.h"
#include "weas/leds_buffer.h"
#include "weas/fifo.h"



// used for testing

class SleepParam : public Param {
public:
  SleepParam(float scale, float linearity, bool log, float lo, float hi)
    : Param(scale, linearity, log, lo, hi) {};
  void set(float /* v */) override {sleep_ms(50);};
  float get() override {return 0.5;}
};



class UIState final : public CtrlHandler {

public:

  UIState(App& app, FIFO& fifo, Codec& codec);
  void handle_ctrl_change(CtrlEvent event) override;
  void per_sample_cb(Codec& codec);

private:

  ATOMIC(RelSource*) source;
  App& app;
  FIFO& fifo;
  LEDsBuffer& leds_buffer;
  BaseLEDsMask* leds_mask;
  enum State {ADJUST, NEXT_PAGE, FREEWHEEL, SOURCE};
  State state = SOURCE;  // start in meta so when we transition knobs are set
  uint32_t INVALID_KNOB = leds_mask->constant(0x08);
  bool started = false;
  uint page = 0;
  uint source_idx = 0;
  uint saved_source_idx = 0;  // used to check if we changed source
  std::array<std::unique_ptr<KnobHandler>, CtrlEvent::N_CTRLS - 1> current_page_knobs = {
    std::make_unique<KnobHandler>(),
    std::make_unique<KnobHandler>(),
    std::make_unique<KnobHandler>()};
  CtrlGate ctrl_gate = CtrlGate({4, 4, 4}, {128, 128, 128});
  KnobHandler source_knob = KnobHandler(1, 1, false, 0, 1);
  Codec& codec;  // used only during startup

  void state_adjust(CtrlEvent event);
  void state_next_page(CtrlEvent event);
  void state_freewheel(CtrlEvent event);
  void state_source(CtrlEvent event);

  uint32_t current_page_mask();
  uint32_t current_source_mask();
  void transition_leds_to(uint32_t mask, bool down);
  void update_source();
  void update_page();
  uint32_t saved_adjust_mask = 0;

  Blank blank = Blank();

};


#endif

