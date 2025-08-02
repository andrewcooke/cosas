
#include "doctest/doctest.h"

#include "cosas/oscillator.h"


TEST_CASE("Oscillator, Wavedex") {
  Wavelib w = Wavelib();
  AbsDexOsc o = AbsDexOsc(4400, w, w.sine_gamma_1);
  CHECK(o.next(1000, 0) == -32417);
  o.get_dex_param().set(w.square_duty_05);
  CHECK(o.next(1000, 0) == -32767);
}


TEST_CASE("Oscillator, Detune") {
  Wavelib w = Wavelib();
  AbsDexOsc o1 = AbsDexOsc(4400, w, w.sine_gamma_1);
  RelDexOsc o2 = RelDexOsc(w, w.sine_gamma_1, o1.get_freq_param(), 1, 1);
  CHECK(o2.next(1000, 0) == -32417);
  o2.get_freq_param().get_det_param().set(0.9f);
  CHECK(o2.next(1000, 0) != -32417);
}


TEST_CASE("Oscillator, AbsDexOsc") {
  Wavelib w = Wavelib();
  AbsDexOsc o = AbsDexOsc(440, w, w.sine_gamma_1);
  // there are table_size samples a second.  a 440hz note takes 1/440s
  // for one cycle, which is sample_rate/440 samples.
  CHECK(o.next(static_cast<int32_t>(0.0f * SAMPLE_RATE / 440), 0) == 0);
  CHECK(abs(o.next(static_cast<int32_t>(0.25f * SAMPLE_RATE / 440), 0) - SAMPLE_MAX) < 1000);
  CHECK(abs(o.next(static_cast<int32_t>(0.25f * SAMPLE_RATE / 440), 0) - 0) < 1000);
  CHECK(abs(o.next(static_cast<int32_t>(0.25f * SAMPLE_RATE / 440), 0) - SAMPLE_MIN) < 1000);
  CHECK(abs(o.next(static_cast<int32_t>(0.25f * SAMPLE_RATE / 440), 0) - 0) < 1000);
}


TEST_CASE("Oscillator, AbsPolyOsc") {
  AbsPolyOsc o = AbsPolyOsc(440, PolyTable::SINE, 0, QUARTER_TABLE_SIZE);
  // there are table_size samples a second.  a 440hz note takes 1/440s
  // for one cycle, which is sample_rate/440 samples.
  CHECK(o.next(static_cast<int32_t>(0.0f * SAMPLE_RATE / 440), 0) == 0);
  CHECK(abs(o.next(static_cast<int32_t>(0.25f * SAMPLE_RATE / 440), 0) - SAMPLE_MAX) < 1000);
  CHECK(abs(o.next(static_cast<int32_t>(0.25f * SAMPLE_RATE / 440), 0) - 0) < 1000);
  CHECK(abs(o.next(static_cast<int32_t>(0.25f * SAMPLE_RATE / 440), 0) - SAMPLE_MIN) < 1000);
  CHECK(abs(o.next(static_cast<int32_t>(0.25f * SAMPLE_RATE / 440), 0) - 0) < 1000);
}
