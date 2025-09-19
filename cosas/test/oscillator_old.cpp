
#include "doctest/doctest.h"

#include "cosas/oscillator_old.h"

int16_t ff1(RelSource& src, uint32_t n) {
  if (n) for (uint32_t i = 0; i < n-1; i++) { src.next(0); }
  return src.next(0);
}

int16_t ff1(AbsSource& src, uint32_t n) {
  if (n) for (uint32_t i = 0; i < n-1; i++) { src.next(0); }
  return src.next(0);
}


TEST_CASE("Oscillator, Wavedex") {
  Wavelib w = Wavelib();
  AbsDexOsc o = AbsDexOsc(4400, w, w.sine_gamma_1);
  CHECK(ff1(o, 1000) == -2025);
  o.get_dex_param().set(w.square_duty_05);
  CHECK(ff1(o, 1000) == -2047);
}


TEST_CASE("Oscillator, Detune") {
  Wavelib w = Wavelib();
  AbsDexOsc o1 = AbsDexOsc(4400, w, w.sine_gamma_1);
  RelDexOsc o2 = RelDexOsc(w, w.sine_gamma_1, o1.get_freq_param(), 1, 1);
  CHECK(ff1(o2, 1000) == -2025);
  o2.get_freq_param().get_det_param().set(0.9f);
  CHECK(ff1(o2, 1000) != -32417);
}


TEST_CASE("Oscillator, AbsDexOsc") {
  Wavelib w = Wavelib();
  AbsDexOsc o = AbsDexOsc(440, w, w.sine_gamma_1);
  // there are table_size samples a second.  a 440hz note takes 1/440s
  // for one cycle, which is sample_rate/440 samples.
  CHECK(ff1(o, static_cast<int32_t>(0.0f * SAMPLE_RATE / 440)) == 128);  // todo - why not 0?
  CHECK(abs(ff1(o, static_cast<int32_t>(0.25f * SAMPLE_RATE / 440)) - SAMPLE_MAX) < 1000);
  CHECK(abs(ff1(o, static_cast<int32_t>(0.25f * SAMPLE_RATE / 440)) - 0) < 1000);
  CHECK(abs(ff1(o, static_cast<int32_t>(0.25f * SAMPLE_RATE / 440)) - SAMPLE_MIN) < 1000);
  CHECK(abs(ff1(o, static_cast<int32_t>(0.25f * SAMPLE_RATE / 440)) - 0) < 1000);
}


TEST_CASE("Oscillator, AbsPolyOsc") {
  AbsPolyOsc o = AbsPolyOsc(440, PolyTable::SINE, 0, QUARTER_TABLE_SIZE);
  // there are table_size samples a second.  a 440hz note takes 1/440s
  // for one cycle, which is sample_rate/440 samples.
  CHECK(ff1(o, static_cast<int32_t>(0.0f * SAMPLE_RATE / 440)) == 128);  // todo - why not 0?
  CHECK(abs(ff1(o, static_cast<int32_t>(0.25f * SAMPLE_RATE / 440)) - SAMPLE_MAX) < 1000);
  CHECK(abs(ff1(o, static_cast<int32_t>(0.25f * SAMPLE_RATE / 440)) - 0) < 1000);
  CHECK(abs(ff1(o, static_cast<int32_t>(0.25f * SAMPLE_RATE / 440)) - SAMPLE_MIN) < 1000);
  CHECK(abs(ff1(o, static_cast<int32_t>(0.25f * SAMPLE_RATE / 440)) - 0) < 1000);
}


TEST_CASE("Oscillator, AbsPolyOsc params") {
  AbsPolyOsc o = AbsPolyOsc(440, PolyTable::SINE, 0, QUARTER_TABLE_SIZE);
  for (size_t shape = 0; shape < PolyTable::N_SHAPES; shape++) {
    o.get_shp_param().set(shape);
    for (size_t asym = 0; asym < PolyTable::N_SHAPES; asym++) {
      o.get_asym_param().set(asym);
      for (size_t offset = 0; offset < HALF_TABLE_SIZE; offset += HALF_TABLE_SIZE / 10) {
        o.get_off_param().set(offset);
        CHECK(o.get_shp_param().get() == shape);
        CHECK(o.get_asym_param().get() == asym);
        CHECK(o.get_off_param().get() == offset);
      }
    }
  }
}
