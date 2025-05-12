\
#ifndef COSA_ENGINE_H
#define COSA_ENGINE_H

import std;
using namespace std;

#include "params.h"
#include "source.h"
#include "lookup.h"
#include "node.h"


// forward decl
class Oscillator;


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

#endif
