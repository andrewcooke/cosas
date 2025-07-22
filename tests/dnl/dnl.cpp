#include <algorithm>
#include <cmath>
#include <numbers>

#include "cosas/maths.h"
#include "weas/cc.h"
#include "weas/leds.h"


class DNL final : public CC {

private:

  static constexpr bool DEBUG = false;
  // static constexpr uint NOISE = 12;  // bits of score to discard
  static constexpr uint NOISE = 4;
  LEDs& leds = LEDs::get();
  Switch sw = Down;
  uint32_t count = 0;
  int32_t score = 0;
  int prev_out = 0;
  uint wtable_idx = 0;
  constexpr static uint wtable_bits = 12;
  constexpr static uint wtable_size = 1 << wtable_bits;
  int16_t wtable[wtable_size] = {};

  void update_switch() {
    Switch sw2 = SwitchVal();
    if (sw2 != sw) {
      score = 0;
      sw = sw2;
    }
  }

  int16_t correct(bool up, int16_t in) {
    uint16_t in_abs = (in + 0x800) & 0x1fff;
    if (up) {
      in = static_cast<int16_t>(fix_dnl_ac_px(in_abs, 10000)) - 0x800;
      // in = static_cast<int16_t>(fix_dnl_ac(in_abs)) - 0x800;
      // in = static_cast<int16_t>(fix_dnl_ac_no_mod(in_abs)) - 0x800;
      // in = static_cast<int16_t>(fix_dnl_cj(in_abs)) - 0x800;
    } else {
      // in = static_cast<int16_t>(fix_dnl_cj(in_abs)) - 0x800;
      // in = static_cast<int16_t>(fix_dnl_ac(in_abs)) - 0x800;
      // in = static_cast<int16_t>(fix_dnl_ac_3fe(in_abs)) - 0x800;
      // in = static_cast<int16_t>(fix_dnl_ac_no_mod(in_abs)) - 0x800;
      in = static_cast<int16_t>(fix_dnl_ac_px(in_abs, 100)) - 0x800;
    }
    return in;
  }

  void compare_and_score(int16_t next_out) {

    // signal sent to 0 and 1
    // should be wired to inputs on 0 and 1 (order not important)

    for (uint lr = 0; lr < 2; lr++) AudioOut(lr, next_out);
    uint chan = KnobVal(Y) < 2048 ? 0 : 1;

    switch (sw) {
    case Up:
      score += abs(correct(true, AudioIn(chan)) - prev_out) - abs(correct(false, AudioIn(chan)) - prev_out);
      break;
    case Middle:
      score += abs(correct(false, AudioIn(chan)) - prev_out) - abs(correct(true, AudioIn(chan)) - prev_out);
      break;
    case Down:
      // flash chan
      leds.set(2 + chan, true);
      // this one is all errors, so should grow faster
      score += abs(correct(true, AudioIn(chan)) - prev_out) + abs(correct(false, AudioIn(chan)) - prev_out);
      break;
    }

    // this displays +ve numbers are bright, -ve as dim
    // NOTE - swapped from previous commit
    // on up switch, upper block of code in correct() is positive so a bright red light means that has more errors
    // on middle switch bottom block of code in correct() is positive to a bright red light means that has more errors
    // so the switch points to the "loser"
    // if up is bright, top is worse
    // if middle is bright, bottom is worse
    // brightness should change switch with the switch
    leds.display7bits(
      static_cast<int16_t>(std::max(-0x7fff,
      static_cast<int>(std::min(
        static_cast<int32_t>(0x7fff), score >> NOISE)))));
  }

  void output_all(int16_t next_out) {
    // raw output on 0 should be wired to input on 0
    // output on 1 will be read/corrected ac/corrected cj depending on switch
    AudioOut(0, next_out);
    int16_t in = AudioIn(0);
    if (sw != Down) in = correct(sw == Middle, in);
    AudioOut(1, in);
  }

  void ProcessSample() override {
    update_switch();
    int16_t next_out = wtable[count % wtable_size];
    if (DEBUG) {
      output_all(next_out);
    } else {
      compare_and_score(next_out);
    }
    prev_out = next_out;
    count++;
  }

public:
  DNL() {
    for (uint i = 0; i < wtable_size; i++)
      // want sawtooth (so no sudden changes) that covers all values
      wtable[i] = static_cast<int16_t>(i < wtable_size / 2 ? i * 2 - 0x800 : 0x1801 - i * 2);
  }
};


int main() {
  DNL dnl;
  dnl.Run();
};
