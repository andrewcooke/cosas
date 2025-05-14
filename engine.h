\
#ifndef COSA_ENGINE_H
#define COSA_ENGINE_H

import std;
using namespace std;

#include "node.h"
#include "oscillator.h"
#include "params.h"


// forward decl
class Oscillator;


// this needs to be simplified and then made more complex again where
// it is required.

/*
class Manager {
  
public:
  
  Manager(bool ext);
  Oscillator& get_oscillator(size_t n) const;
  void set_root(uint16_t freq);
  const AbsoluteFreq& get_root() const;
  bool is_extended() const;
  vector<Node&> build_model(size_t n);

  const size_t n_models = 0;
    
private:

  bool extended;
  AbsoluteFreq* root;
  vector<unique_ptr<Wavetable>> wavetables;
  void init_wavetables();
  vector<unique_ptr<Oscillator>> oscillators;
  void init_oscillators();
          
};
*/


class Manager {

public:

  enum Engine {
    SIMPLE_FM
  };

  Node& build(Engine);
  const Frequency& get_root() const;
  
private:

  void init_wavetables();
  // TODO - templates
  //Oscillator& add_oscillator<FType>(wave_idx, freq_params);
  Node& build_simple_fm();

  size_t sine_start;
  size_t sine_gamma_1;
  size_t square_start;
  size_t square_duty_05;
  size_t saw_start;
  size_t saw_offset_0;
  size_t noise_start;
  size_t noise_smooth_1;
  
  unique_ptr<vector<unique_ptr<Wavetable>>> all_wavetables;
  unique_ptr<vector<unique_ptr<Oscillator>>> current_oscillators;
  unique_ptr<vector<unique_ptr<Node>>> current_nodes;
  
};

#endif
