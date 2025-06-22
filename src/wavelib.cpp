
#include <iostream>

#include "doctest.h"

#include "wavelib.h"


Wavelib::Wavelib()
  : all_wavetables(std::move(std::make_unique<std::vector<std::unique_ptr<Wavetable>>>())) {
  init_wavetables();
}

// these are calculated on startup because large/slow

void Wavelib::init_wavetables() {
  saw_start = all_wavetables->size();
  // do we need both sides?
  for (const auto& offset : {-1.0, -0.5, 0.0, 0.5, 1.0}) {
    if (offset == 0.0) saw_offset_0 = all_wavetables->size();
    all_wavetables->push_back(std::move(std::make_unique<WSaw>(offset)));
  }
  
  sine_start = all_wavetables->size();
  for (const auto& gamma : {4.0, 2.0, 1.0, 0.5, 0.25}) {
    if (gamma == 1.0) sine_gamma_1 = all_wavetables->size();
    all_wavetables->push_back(std::move(std::make_unique<Sine>(gamma)));
  }

  square_start = all_wavetables->size();
  // do we need both sides?
  for (const auto& duty : {0.1, 0.3, 0.5, 0.7, 0.9}) {
    if (duty == 0.5) square_duty_05 = all_wavetables->size();
    all_wavetables->push_back(std::move(std::make_unique<Square>(duty)));
  }

  noise_start = all_wavetables->size();
  // no idea if these smooth values make sense
  for (const auto& smooth : {1, 4, 16, 64, 256}) {
    if (smooth == 1) noise_smooth_1 = all_wavetables->size();
    all_wavetables->push_back(std::move(std::make_unique<Noise>(smooth)));
  }  
}

Wavetable& Wavelib::operator[](size_t idx) {
  return *all_wavetables->at(idx);
}

size_t Wavelib::size() {
  return all_wavetables->size();
}

TEST_CASE("Wavelib") {
  Wavelib w = Wavelib();
  CHECK(w[w.square_duty_05].next(0 << subtick_bits) == sample_max);
  CHECK(w[w.square_duty_05].next(half_table_size << subtick_bits) == sample_max);
  CHECK(w[w.square_duty_05].next((half_table_size+1) << subtick_bits) == sample_min);
  CHECK(w[w.square_duty_05].next((full_table_size-1) << subtick_bits) == sample_min);
  CHECK(w[w.square_duty_05].next(full_table_size << subtick_bits) == sample_max);
}
