
#include <iostream>
#include <memory>
#include <stdexcept>

#include "modulators.h"
#include "engine.h"


Manager::Manager()
  : wavelib(std::move(std::make_unique<Wavelib>())),
    current_nodes(std::move(std::make_unique<std::vector<std::unique_ptr<Node>>>())) {};

const Node& Manager::build(Manager::Engine engine) {
  current_nodes->clear();
  switch(engine) {
  case Manager::Engine::FM:
    return build_fm();
  case Manager::Engine::FM_LFO:
    return build_fm_lfo();
  case Manager::Engine::FM_FB:
    return build_fm_fb();
  case Manager::Engine::FM_FB_FLT:
    return build_fm_fb_flt();
  case Manager::Engine::FM_FMNT:
    return build_fm_fmnt();
  default:
    throw std::domain_error("missing case in Manager::build?");
  }
}

std::tuple<Oscillator&, AbsoluteFreq&> Manager::add_abs_osc(Wavedex wdex, uint16_t f) {
  std::unique_ptr<AbsoluteFreq> freq = std::make_unique<AbsoluteFreq>(f);
  AbsoluteFreq& root = *freq;
  std::unique_ptr<Oscillator> osc = std::make_unique<Oscillator>(wdex, std::move(freq));
  current_nodes->push_back(std::move(osc));
  return {dynamic_cast<Oscillator&>(*current_nodes->back()), root};
}

Oscillator& Manager::add_rel_osc(Wavedex wdex, AbsoluteFreq& root, float ratio, float detune) {
  std::unique_ptr<RelativeFreq> freq = std::make_unique<RelativeFreq>(root, ratio, detune);
  std::unique_ptr<Oscillator> osc = std::make_unique<Oscillator>(wdex, std::move(freq));
  current_nodes->push_back(std::move(osc));
  return dynamic_cast<Oscillator&>(*current_nodes->back());
}

template<typename ModType, typename... Args> ModType& Manager::add_modulator(const Node& nd1, const Node& nd2, Args... args) {
  std::unique_ptr<ModType> mod = std::make_unique<ModType>(nd1, nd2, std::forward<Args>(args)...);
  current_nodes->push_back(std::move(mod));
  return dynamic_cast<ModType&>(*current_nodes->back());
}

template<typename TranType, typename... Args> TranType& Manager::add_transformer(const Node& nd1, Args... args) {
  std::unique_ptr<TranType> tran = std::make_unique<TranType>(nd1, std::forward<Args>(args)...);
  current_nodes->push_back(std::move(tran));
  return dynamic_cast<TranType&>(*current_nodes->back());
}

Constant& Manager::add_constant(uint16_t k) {
  std::unique_ptr<Constant> cons = std::make_unique<Constant>(k);
  current_nodes->push_back(std::move(cons));
  return dynamic_cast<Constant&>(*current_nodes->back());
}

Latch& Manager::add_latch() {
  std::unique_ptr<Latch> lat = std::make_unique<Latch>();
  current_nodes->push_back(std::move(lat));
  return dynamic_cast<Latch&>(*current_nodes->back());
}

const Node& Manager::build_fm() {
  // i don't understand this 3.  is it subtick_bits?
  return build_fm(1.0 / (1 << (phi_fudge_bits - 3)));
}

const Node& Manager::build_fm(float a) {
  auto [car, root] = add_abs_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), 440);
  const Node& mod = add_rel_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), root, 1, 1);
  Amplitude amp = Amplitude(a);
  Balance bal = Balance();
  const ModularFM& fm = add_modulator<ModularFM>(car, mod, amp, bal);
  return fm;
}

TEST_CASE("BuildFM") {
  Manager m = Manager();
  int16_t amp0 = m.build_fm(0).next(50, 0);
  int16_t amp01 = m.build_fm(0.001).next(50, 0);
  CHECK(amp0 == amp01);
  amp0 = m.build_fm(0).next(51, 0);
  amp01 = m.build_fm(0.001).next(51, 0);
  CHECK(amp0 == amp01 - 18);  // exact diff not important, just should not be huge
}

const Node& Manager::build_fm_lfo() {
  return build_fm_lfo(1.0 / (1 << (phi_fudge_bits - 4)));
}

const Node& Manager::build_fm_lfo(float a) {
  auto [car, root] = add_abs_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), 440);
  auto [lfo, lfo_freq] = add_abs_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), 10);
  const Node& mod = add_rel_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), root, 1, 1);
  const Node& am = add_modulator<AM>(lfo, mod);
  Amplitude amp = Amplitude(a);
  Balance bal = Balance();
  const ModularFM& fm = add_modulator<ModularFM>(car, am, amp, bal);
  return fm;
}

const Node& Manager::build_fm_fb() {
  return build_fm_fb(1.0 / (1 << (phi_fudge_bits - 4)));
}

const Node& Manager::build_fm_fb(float a) {
  auto [car, root] = add_abs_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), 440);
  Oscillator& mod = add_rel_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), root, 0.5, 1.1);
  Latch& latch = add_latch();
  MeanFilter& mf = add_transformer<MeanFilter>(latch, MeanFilter::Length(1));
  Merge& mrg = add_modulator<Merge>(mf, mod, Balance(0.5));
  ModularFM& fm = add_modulator<ModularFM>(car, mrg, Amplitude(a), Balance());
  latch.set_source(&fm);
  return latch;
}

const Node& Manager::build_fm_fb_flt() {
  return build_fm_fb(1.0 / (1 << (phi_fudge_bits - 4)));
}

const Node& Manager::build_fm_fb_flt(float a) {
  auto [car, root] = add_abs_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), 440);
  Oscillator& mod = add_rel_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), root, 0.5, 1.1);
  Latch& latch = add_latch();
  Merge& mrg = add_modulator<Merge>(latch, mod, Balance(0.5));
  MeanFilter& flt = add_transformer<MeanFilter>(mrg, MeanFilter::Length(1));
  ModularFM& fm = add_modulator<ModularFM>(car, flt, Amplitude(a), Balance());
  latch.set_source(&fm);
  return latch;
}

const Node& Manager::build_fm_fmnt() {
  auto [car, root] = add_abs_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), 440);
  Latch& latch = add_latch();
  PriorityMerge& mrg = add_transformer<PriorityMerge>(latch, PriorityMerge::Weight(0.5));
  auto [mod1, mod_root] = add_abs_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), 200);
  mrg.add_node(mod1, PriorityMerge::Weight(0.2));
  Oscillator& mod2 = add_rel_osc(Wavedex(*wavelib, wavelib->sine_gamma_1), mod_root, 2, 1);
  mrg.add_node(mod2, PriorityMerge::Weight(0.1));
  Oscillator& mod3 = add_rel_osc(Wavedex(*wavelib, wavelib->noise_start), mod_root, 0.5, 1);
  mrg.add_node(mod3, PriorityMerge::Weight(0.1));
  MeanFilter& flt = add_transformer<MeanFilter>(mrg, MeanFilter::Length(1));
  ModularFM& fm = add_modulator<ModularFM>(car, flt, Amplitude(1), Balance());
  latch.set_source(&fm);
  return latch;
}

