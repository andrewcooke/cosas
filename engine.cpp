
#include <stdexcept>

#include "engine.h"


Node& Manager::build(Manager::Engine engine) {
  current_oscillators->clear();
  current_nodes->clear();
  switch(engine) {
  case Manager::Engine::SIMPLE_FM:
    return build_simple_fm();
  default:
    throw std::domain_error("missing case in Manager::build?");
  }
}

// these are calculated on strartup because large/slow

void Manager::init_wavetables() {
  
  saw_start = all_wavetables->size();
  // do we need both sides?
  for (const auto& offset : {-1.0, -0.5, 0.0, 0.5, 1.0}) {
    if (offset == 0.0) saw_offset_0 = all_wavetables->size();
    all_wavetables->push_back(std::move(std::make_unique<Saw>(offset)));
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

template<typename FreqType, typename... Args> std::tuple<Oscillator&, FreqType&> Manager::add_oscillator(size_t wave_idx, Args... args) {
  Wavetable& wave = *all_wavetables->at(wave_idx);
  std::unique_ptr<FreqType> freq = std::make_unique<FreqType>(std::forward<Args>(args)...);
  FreqType& freq_ref = *freq;
  std::unique_ptr<Oscillator> osc = std::make_unique<Oscillator>(wave, std::move(freq));
  Oscillator& osc_ref = *osc;
  current_oscillators->push_back(std::move(osc));
  // is this ok?  the std::unique_ptr are null.
  return {osc_ref, freq_ref};
	  
}

Node& Manager::build_simple_fm() {
  auto car_root = add_oscillator<AbsoluteFreq>(sine_gamma_1, 440);
  Oscillator& car = get<Oscillator&>(car_root);
  AbsoluteFreq root = get<AbsoluteFreq&>(car_root);
  Oscillator& mod = get<Oscillator&>(add_oscillator<RelativeFreq>(sine_gamma_1, root, 0.5));
  //  Amplitude
  //  std::unique_ptr<MixedFM&> fm = std::make_unique<MixedFm>(car, 
  return *current_nodes->at(0);
}


