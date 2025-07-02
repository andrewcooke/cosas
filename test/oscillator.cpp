
#include "doctest/doctest.h"

#include "cosas/oscillator.h"


TEST_CASE("Oscillator, Wavedex") {
  Wavelib w = Wavelib();
  AbsDexOsc o = AbsDexOsc(4400, w, w.sine_gamma_1);
  CHECK(o.next(1000, 0) == 5800);
  o.get_dex_param().set(w.square_duty_05);
  CHECK(o.next(1000, 0) == 32767);
}


TEST_CASE("Oscillator, Detune") {
  Wavelib w = Wavelib();
  AbsDexOsc o1 = AbsDexOsc(4400, w, w.sine_gamma_1);
  RelDexOsc o2 = RelDexOsc(w, w.sine_gamma_1, o1.get_freq_param(), 1, 1);
  CHECK(o2.next(1000, 0) == 5800);
  o2.get_freq_param().get_det_param().set(0.9);
  CHECK(o2.next(1000, 0) != 5800);
}


TEST_CASE("Oscillator, AbsPolyOsc") {
  AbsPolyOsc o = AbsPolyOsc(440, PolyTable::SINE, 0, QUARTER_TABLE_SIZE);
  CHECK(o.next(0, 0) == 0);
  // there are table_size samples a second.  a 440hz note takes 1/440s
  // for one cycle, which is table_size/440 samples.  so a quarter of
  // a cycle takes quarter_table_size/440. 
  CHECK(o.next((QUARTER_TABLE_SIZE / 440) << SUBTICK_BITS, 0) == SAMPLE_MAX - 1);  // almost
}


TEST_CASE("Oscillator, Frequency") {

}