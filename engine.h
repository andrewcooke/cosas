
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
    SIMPLE_FM,
    SIMPLE_FM_FB
  };

  Manager();
  const Node& build(Engine);
  
private:

  std::tuple<const Oscillator&, AbsoluteFreq&> add_abs_osc(size_t wave_idx, uint16_t freq);
  const Oscillator& add_rel_osc(size_t wave_idx, AbsoluteFreq& root, float ratio, float detune);
  template<typename ModType, typename... Args> const ModType& add_modulator(const Node& nd1, const Node& nd2, Args... args);
  template<typename TranType, typename... Args> const TranType& add_transformer(const Node& nd1, Args... args);
  const Latch& add_latch();
  const Node& build_simple_fm();
  const Node& build_simple_fm_fb();

  std::unique_ptr<Wavelib> wavelib;
  std::unique_ptr<std::vector<std::unique_ptr<Node>>> current_nodes;
  
};

#endif
