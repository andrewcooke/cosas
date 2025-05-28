
#ifndef COSA_ENGINE_H
#define COSA_ENGINE_H

#include <tuple>
#include <vector>
#include <memory>

#include "wavelib.h"
#include "node.h"
#include "oscillator.h"
#include "params.h"


// forward decl
class Oscillator;


class Manager {

public:

  enum Engine {
    FM,
    FM_LFO,
    FM_FB,
    FM_FB_FLT,
    FM_FMNT
  };

  Manager();
  const Node& build(Engine);

  // should be private, but useful for testing
  const Node& build_fm();
  const Node& build_fm(float a);
  const Node& build_fm_lfo();
  const Node& build_fm_lfo(float a);
  const Node& build_fm_fb();
  const Node& build_fm_fb(float a);
  const Node& build_fm_fb_flt();
  const Node& build_fm_fb_flt(float a);
  const Node& build_fm_fmnt();

private:

  std::tuple<Oscillator&, AbsoluteFreq&> add_abs_osc(Wavedex wdex, uint16_t freq);
  Oscillator& add_rel_osc(Wavedex wdex, AbsoluteFreq& root, float ratio, float detune);
  template<typename ModType, typename... Args> ModType& add_modulator(const Node& nd1, const Node& nd2, Args... args);
  template<typename TranType, typename... Args> TranType& add_transformer(const Node& nd1, Args... args);
  Constant& add_constant(uint16_t k);
  Latch& add_latch();

  std::unique_ptr<Wavelib> wavelib;
  std::unique_ptr<std::vector<std::unique_ptr<Node>>> current_nodes;
  
};

#endif
