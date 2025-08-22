#include <algorithm>
#include <cmath>

#include "cosas/dnl.h"
#include "weas/codec.h"
#include "weas/leds.h"

#include <weas/leds_direct.h>


static int32_t sqr(int32_t x) {
  return x * x;
}


// allow two corrections to be compared
// switch up - show one correction (y selects display)
// switch middle - show other connection (y selects display)
// switch down - show comparative score
// "showing" a correction involves:
// * generating a triangle wave on output 0
// * reading triangle wave on input 1
// * displaying raw, raw - expected, corrected - expected, corrected on output 1 (depending on y)
// comparison is such that bright leds indicate "top" has "won"
// "top" refers to upper switch and upper connection in code below
// in addition, the main knob controls frequency.  to the left there's a change in output
// every sample; turning to the right every 2, 4 and 8 samples.

// hardware:
// * connect audio out 0 to audio in 1 (central audio sockets)
// * monitor audio out 1 (selected by Y - see code below)
// * can also monitor audio out1 to see input waveform for comparison (identical to Y to left/low)
// typically you want Y at 3/4 (show error in corrected) and then change switch
// up/middle to compare the two on the oscilloscope.
// switch down to "score".  bright leds indicate switch up (correcn1) is better
// (has lower error / higher likelihood)


class DNL {

private:

  // static constexpr uint NOISE = 12;  // bits of score to discard
  static constexpr uint NOISE = 7;
  LEDsDirect leds = LEDsDirect();
  Codec::SwitchPosition sw = Codec::Down;
  uint32_t count = 0;
  uint slow_bits = 0;
  int64_t score = 0;
  int16_t prev_out = 0;
  uint wtable_idx = 0;
  constexpr static uint wtable_bits = 12;
  constexpr static uint wtable_size = 1 << wtable_bits;
  int16_t wtable[wtable_size] = {};
  ScaledDNL<int, int, int> correcn1 = ScaledDNL(fix_dnl_cj2_pxyz, 28, -11, 0, 4, 1);
  // ScaledDNL<int, int> correcn2 = ScaledDNL(fix_dnl_cj2_pxy, 28, -11, 0, 4);
  ScaledDNL<int, int> correcn2 = ScaledDNL(fix_dnl_cj2_pxy, 28, -11, 0, 4); // best
  // ScaledDNL<int, int> correcn2 = ScaledDNL(fix_dnl_ac_pxy, 25, -10, -10, 3);
  // ScaledDNL<int, int> correcn1 = ScaledDNL(fix_dnl_ac_pxy, 25, -10, -10, 3); // best
  // ScaledDNL<int> correcn2 = ScaledDNL(fix_dnl_cx_px, 26, -10, -6); // best
  // ScaledDNL<int> correcn1 = ScaledDNL(fix_dnl_cj_px, 26, -10, 0);  // best
  // ScaledDNL<> correcn2 = ScaledDNL(static_cast<int16_t (*)(uint16_t)>(nullptr), 26, -11);  // best

  void update_controls(Codec& cc) {
    sw = cc.read_switch();
    if (cc.ctrl_changed(Codec::Switch)) score = 0;
    slow_bits = cc.read_ctrl(Codec::Main) >> 10;  // leave 2 bits
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

  void display(Codec& cc, int16_t prev_out, int16_t raw_in, int16_t fixed_in) {
    uint display = cc.read_ctrl(Codec::Y) / 1024;
    switch (display) {
    case 0:
      cc.write_audio(1, raw_in);
      return;
    case 1:
      cc.write_audio(1, raw_in - prev_out);
      return;
    case 2:
      cc.write_audio(1, fixed_in - prev_out);
      return;
    case 3:
      cc.write_audio(1, fixed_in);
      return;
    }
  }

  void display_or_score(Codec& cc, int16_t prev_out, int16_t raw_in) {

    // in all cases triangle to output 0 and read on input 1
    int16_t fixed_in;

    switch (sw) {
    case Codec::Up:
      fixed_in = correct(true, raw_in);
      display(cc, prev_out, raw_in, fixed_in);
      leds.all(false);
      leds.h2(0, 0x80);
      break;
    case Codec::Middle:
      fixed_in = correct(false, raw_in);
      display(cc, prev_out, raw_in, fixed_in);
      leds.all(false);
      leds.h2(1, 0x80);
      break;
    case Codec::Down:
      // bright if upper wins so lower should be larger
      score += sqr(correct(false, raw_in) - prev_out) - sqr(correct(true, raw_in) - prev_out);
      leds.display7bits(
        static_cast<int16_t>(std::max(-0x7fff,
        static_cast<int>(std::min(
          static_cast<int64_t>(0x7fff), score >> NOISE)))));
    }
  }

public:

  void per_sample_cb(Codec& cc) {
    update_controls(cc);
    int16_t next_out = wtable[(count >> slow_bits) % wtable_size];
    cc.write_audio(0, next_out);
    int16_t raw_in = cc.read_audio(1);
    display_or_score(cc, prev_out, raw_in);
    prev_out = next_out;
    count++;
  }

  DNL() {
    for (uint i = 0; i < wtable_size; i++)
      // want sawtooth (so no sudden changes) that covers all values
      wtable[i] = static_cast<int16_t>(i < wtable_size / 2 ? i * 2 - 0x800 : 0x1801 - i * 2);
  }
};


int main() {
  DNL dnl;
  Codec& cc = CodecFactory<3, CODEC_SAMPLE_48>::get();  // actual freq lower for >1 bits oversample
  cc.set_adc_correction_and_scale(CODEC_NULL_CORRECTION);
  cc.select_adc_correction(0);
  cc.select_adc_scale(0);
  cc.set_per_sample_cb([&](Codec& c){dnl.per_sample_cb(c);});
  cc.start();
};
