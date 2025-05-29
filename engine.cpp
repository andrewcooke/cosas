
#include <iostream>
#include <memory>
#include <stdexcept>

#include "modulators.h"
#include "engine.h"


Manager::Manager()
  : wavelib(std::move(std::make_unique<Wavelib>())),
    current_nodes(std::move(std::make_unique<std::vector<std::unique_ptr<Node>>>())),
    current_params(std::move(std::make_unique<std::vector<std::unique_ptr<Param>>>())) {};

const Node& Manager::build(Manager::Engine engine) {
  current_nodes->clear();
  switch(engine) {
  case Manager::Engine::FM:
    return build_fm();
  case Manager::Engine::FM_LFO:
    return build_fm_lfo();
  case Manager::Engine::FM_FB:
    return build_fm_fb();
  case Manager::Engine::FM_FMNT:
    return build_fm_fmnt();
  default:
    throw std::domain_error("missing case in Manager::build?");
  }
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

template<typename ParamType, typename... Args> ParamType& Manager::add_param(Args... args) {
  std::unique_ptr<ParamType> param = std::make_unique<ParamType>(std::forward<Args>(args)...);
  current_params->push_back(std::move(param));
  return dynamic_cast<ParamType&>(*current_params->back());
}

std::tuple<Wavedex&, AbsoluteFreq&, Oscillator&> Manager::add_abs_osc(size_t widx, float frq) {
  current_params->push_back(std::move(std::make_unique<Wavedex>(*wavelib, widx)));
  Wavedex& w = dynamic_cast<Wavedex&>(*current_params->back());
  AbsoluteFreq& f = add_param<AbsoluteFreq>(frq);
  current_nodes->push_back(std::move(std::make_unique<Oscillator>(w, f)));
  Oscillator& o = dynamic_cast<Oscillator&>(*current_nodes->back());
  return {w, f, o};
}

std::tuple<Wavedex&, RelativeFreq&, Oscillator&> Manager::add_rel_osc(size_t widx, AbsoluteFreq& root, float r, float d) {
  std::unique_ptr<Wavedex> wdex = std::make_unique<Wavedex>(*wavelib, widx);
  current_params->push_back(std::move(wdex));
  Wavedex& w = dynamic_cast<Wavedex&>(*current_params->back());
  current_params->push_back(std::move(std::make_unique<RelativeFreq>(root, r, d)));
  RelativeFreq& f = static_cast<RelativeFreq&>(*current_params->back());
  current_nodes->push_back(std::move(std::make_unique<Oscillator>(w, f)));
  Oscillator& o = dynamic_cast<Oscillator&>(*current_nodes->back());
  return {w, f, o};
}

Constant& Manager::add_constant(uint16_t k) {
  std::unique_ptr<Constant> cons = std::make_unique<Constant>(k);
  current_nodes->push_back(std::move(cons));
  return dynamic_cast<Constant&>(*current_nodes->back());
}

std::tuple<Amplitude&, Balance&, ModularFM&> Manager::add_fm(Node& c, Node& m, float amp) {
  Amplitude& a = add_param<Amplitude>(amp);
  Balance& b = add_param<Balance>();
  ModularFM& fm = add_modulator<ModularFM>(c, m, a, b);
  return {a, b, fm};
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

const Node& Manager::build_fm(float amp) {
  auto [cw, cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440);
  auto [mw, mf, m] = add_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  auto [a, b, fm] = add_fm(c, m, amp);
  // TODO - panes
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

const Node& Manager::build_fm_lfo(float amp) {
  auto [cw, cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440);
  auto [mw, mf, m] = add_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  auto [lw, lf, l] = add_abs_osc(wavelib->sine_gamma_1, 1);
  // TODO - need gain for lfo?
  Node& am = add_modulator<AM>(l, m);
  auto [a, b, fm] = add_fm(c, am, amp);
  return fm;
}

const Node& Manager::build_fm_fb() {
  return build_fm_fb(1.0 / (1 << (phi_fudge_bits - 4)));
}

const Node& Manager::build_fm_fb(float amp) {
  auto [cw, cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440);
  auto [mw, mf, m] = add_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  Latch& latch = add_latch();
  MeanFilter::Length l = add_param<MeanFilter::Length>(1);
  MeanFilter& flt = add_transformer<MeanFilter>(latch, l);
  Balance& mb = add_param<Balance>(0.5);
  Merge& mrg = add_modulator<Merge>(flt, m, mb);
  auto [a, b, fm] = add_fm(c, mrg, amp);
  latch.set_source(&fm);
  return latch;
}

// this is via feedback which might be crazy
const Node& Manager::build_fm_fmnt() {
  auto [cw, cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440);
  auto [mw, mf, m] = add_abs_osc(wavelib->sine_gamma_1, 100);
  Latch& latch = add_latch();
  MeanFilter::Length& l = add_param<MeanFilter::Length>(1);
  MeanFilter& flt = add_transformer<MeanFilter>(latch, l);
  PriorityMerge::Weight& w0 = add_param<PriorityMerge::Weight>(0.5);
  PriorityMerge& mrg = add_transformer<PriorityMerge>(flt, w0);
  auto [mw1, mf1, m1] = add_rel_osc(wavelib->sine_gamma_1, mf, 2, 1);
  PriorityMerge::Weight& w1 = add_param<PriorityMerge::Weight>(0.1);
  mrg.add_node(m1, w1);
  auto [mw2, mf2, m2] = add_rel_osc(wavelib->sine_gamma_1, mf, 3, 1);
  PriorityMerge::Weight& w2 = add_param<PriorityMerge::Weight>(0.1);
  mrg.add_node(m2, w2);
  // could add more in parallel here?
  auto [a, b, fm] = add_fm(c, mrg, 1);
  latch.set_source(&fm);
  return latch;
}

