
#include <algorithm>
#include <cmath>
#include <numbers>

#include "weas/cc.h"
#include "weas/leds.h"


class Diagnostics final {

private:

  // detect and display changes, or identify source of previous change

  LEDs& leds = LEDs::get();

  constexpr static uint NONE = 999;
  uint prev_change = NONE;
  int32_t recent = 0;

  constexpr static uint noise_knobs = 4;
  constexpr static uint n_knobs = 3;
  uint32_t knobs[2][n_knobs] = {};
  constexpr static uint noise_switches = 0;
  constexpr static uint n_switches = 1;
  uint32_t switches[2][n_switches] = {};
  constexpr static uint noise_adcs = 7;
  constexpr static uint n_adcs = 4;
  uint32_t adcs[2][n_adcs] = {};
  constexpr static uint noise_pulses = 0;
  constexpr static uint n_pulses = 1;
  bool pulses[2][n_pulses] = {};

  constexpr static uint n_all = n_knobs + n_switches + n_adcs + n_pulses;

  uint32_t count = 0;
  constexpr static uint wtable_bits = 12;
  constexpr static uint wtable_size = 1 << wtable_bits;
  int16_t wtable[wtable_size] = {};

  void save_current(CC& cc) {
    for (uint i = 0; i < n_knobs; i++) {
      knobs[1][i] = knobs[0][i];
      // for some reason knob values are signed integers, but we
      // need to display unsigned so cast here
      knobs[0][i] = cc.KnobVal(static_cast<CC::Knob>(i)) >> noise_knobs;
    }
    switches[1][0] = switches[0][0];
    switches[0][0] = cc.SwitchVal();
    for (uint i = 0; i < n_adcs; i++) {
      adcs[1][i] = adcs[0][i];
      adcs[0][i] = (i < 2 ? cc.AudioIn(i) : cc.CVIn(i - 2)) >> noise_adcs;
    }
    for (uint i = 0; i < n_pulses; i++) {
      pulses[1][i] = pulses[0][i];
      pulses[0][i] = cc.PulseIn(i);
    }
  }

  bool changed(uint idx) const {
    if (idx < n_knobs) return knobs[0][idx] != knobs[1][idx];
    idx -= n_knobs;
    if (idx < n_switches) return switches[0][idx] != switches[1][idx];
    idx -= n_switches;
    if (idx < n_adcs) return adcs[0][idx] != adcs[1][idx];
    idx -= n_adcs;
    if (idx < n_pulses) return pulses[0][idx] != pulses[1][idx];
    // idx -= n_pulses;
    return false;
  }

  uint next_change(uint old_idx) const {
    uint new_idx = old_idx == NONE ? 0 : old_idx + 1;
    for (uint i = 0; i < n_all; i++) {
      if (new_idx == n_all) new_idx = 0;
      if (changed(new_idx)) return new_idx;
      new_idx++;
    }
    return NONE;
  }

  void identify(uint idx) {
    leds.all(0x40u);
    if (idx < n_knobs) {
      switch(idx) {
      case static_cast<uint>(CC::Main):
        leds.sq4(0, 0xffu);
        return;
      case static_cast<uint>(CC::X):
        leds.v2(2, 0xffu);
        return;
      case static_cast<uint>(CC::Y):
        leds.sq4(1, 0xffu);
        return;
      default:
        return;
      }
    }
    idx -= n_knobs;
    if (idx < n_switches) {
      leds.v2(1, 0xffu);
      return;
    }
    idx -= n_switches;
    if (idx < n_adcs) {
      leds.on(idx);  // swap audio l/r
      return;
    }
    idx -= n_adcs;
    if (idx < n_pulses) {
      leds.on(idx + 4);
      return;
    }
    // idx -= n_pulses;
  }

  void display(CC& cc, uint idx) {
    if (idx < n_knobs) {
      leds.columns12bits(static_cast<uint16_t>(cc.KnobVal(static_cast<CC::Knob>(idx))));
      return;
    }
    idx -= n_knobs;
    if (idx < n_switches) {
      leds.all(false);
      leds.h2(static_cast<uint>(cc.SwitchVal()), 0xffu);
      return;
    }
    idx -= n_switches;
    if (idx < n_adcs) {
      leds.columns12bits(idx < 2 ? cc.AudioIn(idx) : cc.CVIn(idx - 2));
    }
    idx -= n_adcs;
    if (idx < n_pulses) {
      for (uint i = 0; i < leds.N; i++) {
        if (i == idx || i == idx + 4) leds.on(i);
        else leds.off(i);
      }
      return;
    }
    idx -= n_pulses;
    return;
  }

  bool pulse(uint n) const {
    return (count & (1 << n)) && ! ((count - 1) & (1 << n));
  }

  void write_out(CC& cc) {
    for (uint i = 0; i < 4; i++) {
      uint idx = (count >> (i + 3)) & (wtable_size - 1);
      if (i < 2) cc.AudioOut(i, wtable[idx]);
      else cc.CVOut(i - 2, wtable[idx]);
    }
    cc.PulseOut(0, pulse(11));;
    cc.PulseOut(1, !pulse(12));
    count++;
  }

  static int delay(const uint prev_change) {
    if (prev_change == n_knobs) return 30000;  // switch
    if (prev_change == n_all - 1) return 100;  // pulse
    return 2000;  // default
  }

public:

  void ProcessSample(CC& cc) {
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

  Diagnostics() {
    for (uint i = 0; i < wtable_size; i++)
      wtable[i] = static_cast<int16_t>(2047 * sin(2 * std::numbers::pi * i / wtable_size));
  }

};


int main()
{
  Diagnostics diag;
  CC& cc = CC::get_instance();
  cc.set_callback([&](CC& cc){diag.ProcessSample(cc);});
  cc.Run();
};

