
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


Node& build_simple_fm(vector<Oscillator>& current_oscillators, vector<Node>& current_nodes) {
  return current_nodes.at(0);
}


Manager::Manager()
  : current_oscillators(move(make_unique<vector<Oscillator>>())),
    current_nodes(move(make_unique<vector<Node>>())) {};

const Frequency& Manager::get_root() const {
  // first oscillator is root
  return current_oscillators->at(0).get_frequency();
}

Node& Manager::build(Manager::Engine engine) {
  current_oscillators->clear();
  current_nodes->clear();
  switch(engine) {
  case Manager::Engine::SIMPLE_FM:
    return build_simple_fm(*current_oscillators, *current_nodes);
  default:
    throw domain_error("missing case in Manager::build?");
  }
}


