
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
    FM_MOD,
    FM_FB,
    FM_FB_FLT
  };

  Manager();
  const Node& build(Engine);
  
private:

  std::tuple<const Oscillator&, AbsoluteFreq&> add_abs_osc(Wavedex wdex, uint16_t freq);
  const Oscillator& add_rel_osc(Wavedex wdex, AbsoluteFreq& root, float ratio, float detune);
  template<typename ModType, typename... Args> const ModType& add_modulator(const Node& nd1, const Node& nd2, Args... args);
  template<typename TranType, typename... Args> const TranType& add_transformer(const Node& nd1, Args... args);
  Constant& add_constant(uint16_t k);
  const Latch& add_latch();

  const Node& build_fm();
  const Node& build_fm(float a);
  const Node& build_fm_mod();
  const Node& build_fm_mod(float a);
  const Node& build_fm_fb();
  const Node& build_fm_fb(float a);
  const Node& build_fm_fb_flt();
  const Node& build_fm_fb_flt(float a);

  std::unique_ptr<Wavelib> wavelib;
  std::unique_ptr<std::vector<std::unique_ptr<Node>>> current_nodes;
  
};

#endif
