#include <algorithm>
#include <cmath>
#include <numbers>

#include "cosas/maths.h"
#include "weas/cc.h"
#include "weas/leds.h"


static int32_t sqr(int32_t x) {
  return x * x;
}


// allow two corrections to be compared
// switch up - show one correction (y selects display)
// switch middle - show other connection (y selects display)
// switch down - show comparative score
// "showing" a corcetion involves:
// * generating a triangle wave on output 0
// * reading triangle wave on input 1
// * displaying raw, raw - expected, corrected - expected, corrected on output 1 (depending on y)
// comparison is such that bright leds indicate "top" has "won"
// "top" refers to upper switch and upper connection in code below

class DNL final : public CC {

private:

  // static constexpr uint NOISE = 12;  // bits of score to discard
  static constexpr uint NOISE = 4;
  static constexpr uint SLOW = 3;  // slow down output freq
  LEDs& leds = LEDs::get();
  Switch sw = Down;
  uint32_t count = 0;
  int32_t score = 0;
  int prev_out = 0;
  uint wtable_idx = 0;
  constexpr static uint wtable_bits = 12;
  constexpr static uint wtable_size = 1 << wtable_bits;
  int16_t wtable[wtable_size] = {};
  // ScaledDNL<int, int> correcn1 = ScaledDNL(fix_dnl_ac_pxy, 25, -10, -10, 3); // best
  // ScaledDNL<int> correcn2 = ScaledDNL(fix_dnl_cx_px, 26, -10, -6); // best
  ScaledDNL<int> correcn1 = ScaledDNL(fix_dnl_cj_px, 26, -10, 0);  // best
  ScaledDNL<> correcn2 = ScaledDNL(static_cast<int16_t (*)(uint16_t)>(nullptr), 26, -11);  // best

  void update_switch() {
    Switch sw2 = SwitchVal();
    if (sw2 != sw) {
      score = 0;
      sw = sw2;
    }
  }

  int16_t correct(bool top, int16_t in) {
    uint16_t in_abs = (in + 0x800) & 0x1fff;
    if (top) {
      in = correcn1(in_abs) - 0x800;
    } else {
      in = correcn2(in_abs) - 0x800;
    }
    return in;
  }

  void display(int16_t prev_out, int16_t raw, int16_t corrected) {
    uint display = KnobVal(Y) / 1024;
    switch (display) {
    case 0:
      AudioOut(1, raw);
      return;
    case 1:
      AudioOut(1, raw - prev_out);
      return;
    case 2:
      AudioOut(1, corrected - prev_out);
      return;
    case 3:
      AudioOut(1, corrected);
      return;
    }
  }

  void display_or_score(int16_t prev_out, int16_t next_out) {

    // in all cases triangle to output 0 and read on input 1
    AudioOut(0, next_out);
    int16_t raw = AudioIn(1);
    int16_t corrected;

    switch (sw) {
    case Up:
      corrected = correct(true, raw);
      display(prev_out, raw, corrected);
      break;
    case Middle:
      corrected = correct(false, raw);
      display(prev_out, raw, corrected);
      break;
    case Down:
      // bright if upper wins so lower should be arger
      score += sqr(correct(false, raw) - prev_out) - sqr(correct(true, raw) - prev_out);
      leds.display7bits(
        static_cast<int16_t>(std::max(-0x7fff,
        static_cast<int>(std::min(
          static_cast<int32_t>(0x7fff), score >> NOISE)))));
    }
  }

  void ProcessSample() override {
    update_switch();
    int16_t next_out = wtable[(count >> SLOW) % wtable_size];
    display_or_score(prev_out, next_out);
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
