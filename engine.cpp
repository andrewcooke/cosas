
#include <iostream>
#include <memory>
#include <stdexcept>

#include "modulators.h"
#include "engine.h"


Manager::Manager()
  : all_wavetables(std::move(std::make_unique<std::vector<std::unique_ptr<Wavetable>>>())),
    current_nodes(std::move(std::make_unique<std::vector<std::unique_ptr<Node>>>())) {
  init_wavetables();
}

const Node& Manager::build(Manager::Engine engine) {
  current_nodes->clear();
  switch(engine) {
  case Manager::Engine::SIMPLE_FM:
    return build_simple_fm();
  case Manager::Engine::SIMPLE_FM_FB:
    return build_simple_fm_fb();
  default:
    throw std::domain_error("missing case in Manager::build?");
  }
}

// these are calculated on startup because large/slow

void Manager::init_wavetables() {
  
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

std::tuple<const Oscillator&, AbsoluteFreq&> Manager::add_abs_osc(size_t wave_idx, uint16_t f) {
  Wavetable& wave = *all_wavetables->at(wave_idx);
  std::unique_ptr<AbsoluteFreq> freq = std::make_unique<AbsoluteFreq>(f);
  AbsoluteFreq& root = *freq;
  std::unique_ptr<Oscillator> osc = std::make_unique<Oscillator>(wave, std::move(freq));
  current_nodes->push_back(std::move(osc));
  return {dynamic_cast<Oscillator&>(*current_nodes->back()), root};
}

const Oscillator& Manager::add_rel_osc(size_t wave_idx, AbsoluteFreq& root, float ratio, float detune) {
  Wavetable& wave = *all_wavetables->at(wave_idx);
  std::unique_ptr<RelativeFreq> freq = std::make_unique<RelativeFreq>(root, ratio, detune);
  std::unique_ptr<Oscillator> osc = std::make_unique<Oscillator>(wave, std::move(freq));
  current_nodes->push_back(std::move(osc));
  return dynamic_cast<Oscillator&>(*current_nodes->back());
}

template<typename ModType, typename... Args> const ModType& Manager::add_modulator(const Node& nd1, const Node& nd2, Args... args) {
  std::unique_ptr<ModType> mod = std::make_unique<ModType>(nd1, nd2, std::forward<Args>(args)...);
  current_nodes->push_back(std::move(mod));
  return dynamic_cast<ModType&>(*current_nodes->back());
}

template<typename TranType, typename... Args> const TranType& Manager::add_transformer(const Node& nd1, Args... args) {
  std::unique_ptr<TranType> tran = std::make_unique<TranType>(nd1, std::forward<Args>(args)...);
  current_nodes->push_back(std::move(tran));
  return dynamic_cast<TranType&>(*current_nodes->back());
}

const Latch& Manager::add_latch() {
  std::unique_ptr<Latch> lat = std::make_unique<Latch>();
  current_nodes->push_back(std::move(lat));
  return dynamic_cast<Latch&>(*current_nodes->back());
}

const Node& Manager::build_simple_fm() {
  auto [car, root] = add_abs_osc(sine_gamma_1, 440);
  const Node& mod = add_rel_osc(sine_gamma_1, root, 0.5, 1.1);
  Amplitude amp = Amplitude(100);
  Balance bal = Balance();
  const ModularFM& fm = add_modulator<ModularFM>(car, mod, amp, bal);
  return fm;
}

const Node& Manager::build_simple_fm_fb() {
  auto [car, root] = add_abs_osc(sine_gamma_1, 440);
  const Oscillator& mod = add_rel_osc(sine_gamma_1, root, 0.5, 1.1);
  const Latch& latch = add_latch();
  const Merge& mrg = add_modulator<Merge>(latch, mod, Balance(0.5));
  const ModularFM& fm = add_modulator<ModularFM>(car, mrg, Amplitude(), Balance());
  latch.set_source(&fm);
  return latch;
}

