
#include "engine.h"


/*
Manager::Manager(bool ext) : extended(ext) {
  init_wavetables();
  init_oscillators();
}

void Manager::init_wavetables() {
  wavetables.push_back(make_unique<Sine>(Sine()));
  wavetables.push_back(make_unique<Square>(Square()));
}

void Manager::init_oscillators() {
  
  for (size_t i = 0; i < max_oscillators; i++) {
    unique_ptr<Amplitude> amplitude = make_unique<Amplitude>(1);
    // the first frequency is absolute; the rest are relative
    if (i == 0) {
      unique_ptr<AbsoluteFreq> f = make_unique<AbsoluteFreq>(440);
      root = &*f;  // saved here for ease of access, but not the owner
      oscillators.push_back(move(make_unique<Oscillator>
				 (*wavetables.at(0), move(f), move(amplitude))));
    } else {
      oscillators.push_back(move(make_unique<Oscillator>
				 (*wavetables.at(0),
				  move(make_unique<RelativeFreq>(oscillators.at(0)->get_frequency(), 1, extended)),
				  move(amplitude))));
    }
  }
}

Oscillator& Manager::get_oscillator(size_t n) const {
  return *oscillators.at(n);
}

void Manager::set_root(uint16_t freq) {
  // unlike other frequencies / oscillators, the root (from which all
  // relative values are calculated)  must stay the "same" instance.
  // so it must be updated in-place so that relative changes work.
  root->set(freq);
}

const AbsoluteFreq& Manager::get_root() const {
  return *root;
}

bool Manager::is_extended() const {
  return extended;
}
*/


const Frequency& Manager::get_root() const {
  // first oscillator is root
  return current_oscillators->at(0)->get_frequency();
}

Node& Manager::build(Manager::Engine engine) {
  current_oscillators->clear();
  current_nodes->clear();
  switch(engine) {
  case Manager::Engine::SIMPLE_FM:
    return build_simple_fm();
  default:
    throw domain_error("missing case in Manager::build?");
  }
}

// these are calculated on strartup because large/slow

void Manager::init_wavetables() {
  
  saw_start = all_wavetables->size();
  // do we need both sides?
  for (const auto& offset : {-1.0, -0.5, 0.0, 0.5, 1.0}) {
    if (offset == 0.0) saw_offset_0 = all_wavetables->size();
    all_wavetables->push_back(move(make_unique<Saw>(offset)));
  }
  
  sine_start = all_wavetables->size();
  for (const auto& gamma : {4.0, 2.0, 1.0, 0.5, 0.25}) {
    if (gamma == 1.0) sine_gamma_1 = all_wavetables->size();
    all_wavetables->push_back(move(make_unique<Sine>(gamma)));
  }

  square_start = all_wavetables->size();
  // do we need both sides?
  for (const auto& duty : {0.1, 0.3, 0.5, 0.7, 0.9}) {
    if (duty == 0.5) square_duty_05 = all_wavetables->size();
    all_wavetables->push_back(move(make_unique<Square>(duty)));
  }

  noise_start = all_wavetables->size();
  // no idea if these smooth values make sense
  for (const auto& smooth : {1, 4, 16, 64, 256}) {
    if (smooth == 1) noise_smooth_1 = all_wavetables->size();
    all_wavetables->push_back(move(make_unique<Noise>(smooth)));
  }  

}

template<typename FreqType, typename Args> Oscillator& Manager::add_oscillator(size_t wave_idx, Args&& args) {
  Wavetable& wave = *all_wavetables->at(wave_idx);
  unique_ptr<FreqType> freq = make_unique<FreqType>(forward<Args>(args));
  unique_ptr<Oscillator> osc = make_unique<Oscillator>(wave, move(freq));
  current_oscillators->push_back(move(osc));
  // osc is empty now, so pull from list
  return *current_oscillators->at(current_oscillators->size()-1);
}

Node& Manager::build_simple_fm() {
  Oscillator& root = add_oscillator<AbsoluteFreq>(sine_gamma_1, 440);

  return *current_nodes->at(0);
}


