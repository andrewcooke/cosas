
#ifndef COSA_ENGINE_H
#define COSA_ENGINE_H

#include <tuple>
#include <vector>
#include <memory>

#include "node.h"
#include "oscillator.h"
#include "params.h"


// forward decl
class Oscillator;


class Manager {

public:

  enum Engine {
    SIMPLE_FM
  };

  Node& build(Engine);
  
private:

  void init_wavetables();
  template<typename FreqType, typename... Args> std::tuple<Oscillator&, FreqType&> add_oscillator(size_t wave_idx, Args... args);
  Node& build_simple_fm();

  size_t sine_start;
  size_t sine_gamma_1;
  size_t square_start;
  size_t square_duty_05;
  size_t saw_start;
  size_t saw_offset_0;
  size_t noise_start;
  size_t noise_smooth_1;

  std::unique_ptr<std::vector<std::unique_ptr<Wavetable>>> all_wavetables;
  std::unique_ptr<std::vector<std::unique_ptr<Oscillator>>> current_oscillators;
  std::unique_ptr<std::vector<std::unique_ptr<Node>>> current_nodes;
  
};

#endif
