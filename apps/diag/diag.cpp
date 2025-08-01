
#include <algorithm>
#include <cmath>
#include <numbers>

#include "cosas/dnl.h"
#include "weas/codec.h"
#include "weas/leds.h"
#include "weas/leds_direct.h"


constexpr float FDIV = 44.1f;
constexpr uint SAMPLE_FREQ = static_cast<uint>(CC_SAMPLE_44_1 / FDIV);


class Diagnostics final {

private:

  // detect and display changes, or identify source of previous change

  LEDs& leds = LEDs::get();
  LEDsDirect leds_direct;

  constexpr static uint NONE = 999;
  uint prev_change = NONE;
  int32_t recent = 0;

  enum When { Now, Prev };
  constexpr static uint N_WHEN = Prev + 1;
  constexpr static uint THRESH_KNOBS = 4;
  uint32_t knobs[N_WHEN][Codec::N_KNOBS] = {};
  constexpr static uint THRESH_ADCS = 7;
  constexpr static uint N_ADCS = 4;
  uint32_t adcs[N_WHEN][N_ADCS] = {};
  constexpr static uint THRESH_PULSES = 0;
  constexpr static uint N_PULSES = 2;
  bool pulses[N_WHEN][N_PULSES] = {};
  constexpr static uint N_ALL = Codec::N_KNOBS + N_ADCS + N_PULSES;

  uint32_t count = 0;
  constexpr static uint WTABLE_BITS = 12;
  constexpr static uint WTABLE_SIZE = 1 << WTABLE_BITS;
  int16_t wtable[WTABLE_SIZE] = {};

  void save_current(Codec& cc) {
    for (uint knob = 0; knob < Codec::N_KNOBS; knob++) {
      knobs[Prev][knob] = knobs[Now][knob];
      // for some reason knob values are signed integers, but we
      // need to display unsigned so cast here
      knobs[Now][knob] = cc.read_knob(static_cast<Codec::Knob>(knob)) >> (knob == Codec::Switch ? 0 : THRESH_KNOBS);
    }
    for (uint adc = 0; adc < N_ADCS; adc++) {
      adcs[Prev][adc] = adcs[Now][adc];
      adcs[Now][adc] = (adc < 2 ? cc.read_audio(adc) : cc.read_cv(adc - 2)) >> THRESH_ADCS;
    }
    for (uint pulse = 0; pulse < N_PULSES; pulse++) {
      pulses[Prev][pulse] = pulses[Now][pulse];
      pulses[Now][pulse] = cc.read_pulse(pulse);
    }
  }

  bool changed(uint idx) const {
    if (idx < Codec::N_KNOBS) return knobs[0][idx] != knobs[1][idx];
    idx -= Codec::N_KNOBS;
    if (idx < N_ADCS) return adcs[0][idx] != adcs[1][idx];
    idx -= N_ADCS;
    if (idx < N_PULSES) return pulses[0][idx] != pulses[1][idx];
    return false;
  }

  uint next_change(uint old_idx) const {
    uint new_idx = old_idx == NONE ? 0 : old_idx + 1;
    for (uint i = 0; i < N_ALL; i++) {
      if (new_idx == N_ALL) new_idx = 0;
      if (changed(new_idx)) return new_idx;
      new_idx++;
    }
    return NONE;
  }

  void identify(uint idx) {
    leds.all(0x40u);
    if (idx < Codec::N_KNOBS) {
      switch(idx) {
      case static_cast<uint>(Codec::Main):
        leds_direct.sq4(0, 0xffu);
        return;
      case static_cast<uint>(Codec::X):
        leds_direct.v2(2, 0xffu);
        return;
      case static_cast<uint>(Codec::Y):
        leds_direct.sq4(1, 0xffu);
        return;
      case static_cast<uint>(Codec::Switch):
        leds_direct.v2(1, 0xffu);
        return;
      default:
        return;
      }
    }
    idx -= Codec::N_KNOBS;
    if (idx < N_ADCS) {
      leds.on(idx);  // swap audio l/r
      return;
    }
    idx -= N_ADCS;
    if (idx < N_PULSES) {
      leds.on(idx + 4);
      return;
    }
  }

  void display(Codec& cc, uint idx) {
    if (idx < Codec::N_KNOBS) {
      if (idx == Codec::Switch) {
        leds.all(false);
        leds_direct.h2(static_cast<uint>(cc.read_switch()), 0xffu);
      } else {
        leds_direct.columns12bits(static_cast<uint16_t>(cc.read_knob(static_cast<Codec::Knob>(idx))));
      }
      return;
    }
    idx -= Codec::N_KNOBS;
    if (idx < N_ADCS) {
      leds_direct.columns12bits(idx < 2 ? cc.read_audio(idx) : cc.read_cv(idx - 2));
    }
    idx -= N_ADCS;
    if (idx < N_PULSES) {
      for (uint i = 0; i < leds.N; i++) {
        if (i == idx || i == idx + 4) leds.on(i);
        else leds.off(i);
      }
    }
  }

  bool pulse(uint n) const {
    return (count & (1 << n)) && ! ((count - 1) & (1 << n));
  }

  void write_out(Codec& cc) {
    for (uint i = 0; i < 4; i++) {
      uint idx = (count >> (i + 3)) & (WTABLE_SIZE - 1);
      if (i < 2) cc.write_audio(i, wtable[idx]);
      else cc.write_cv(i - 2, wtable[idx]);
    }
    cc.write_pulse(0, pulse(11));;
    cc.write_pulse(1, !pulse(12));
    count++;
  }

  static int delay(const uint prev_change) {
    if (prev_change == Codec::N_KNOBS - 1) return static_cast<int>(30000 / FDIV);  // switch
    if (prev_change == N_ALL - 1) return static_cast<int>(100 / FDIV);  // pulse
    return 2000;  // default
  }

public:

  void ProcessSample(Codec& cc) {
    write_out(cc);
    save_current(cc);
    if (prev_change != NONE && ((recent-- > 0) || changed(prev_change))) {
      if (changed(prev_change)) recent = delay(prev_change);
      display(cc, prev_change);
    } else {
      uint current_change = next_change(prev_change);
      if (current_change == NONE && prev_change != NONE) identify(prev_change);
      if (current_change != NONE) {
        prev_change = current_change;
        recent = delay(prev_change);
      }
    }
  }

  Diagnostics() : leds_direct() {
    for (uint i = 0; i < WTABLE_SIZE; i++)
      wtable[i] = static_cast<int16_t>(2047 * sin(2 * std::numbers::pi * i / WTABLE_SIZE));
  }

};

int main() {
  Diagnostics diag;
  Codec& cc = CodecFactory<1, SAMPLE_FREQ>::get();
  cc.set_per_sample_cb([&](Codec& c){diag.ProcessSample(c);});
  cc.set_adc_correction(fix_dnl);
  cc.select_adc_correction(Codec::All);
  cc.set_adc_scale(true);
  cc.start();
};

