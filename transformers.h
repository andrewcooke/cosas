
#ifndef COSA_TRANSFORMERS_H
#define COSA_TRANSFORMERS_H

#include <list>

#include "constants.h"
#include "params.h"
#include "node.h"
#include "lookup.h"


// these have one input (modulators have two)


class Transformer : public Node {

protected:

  Transformer(Node& nd) : node(nd) {};
  Node& node;
  
};


class Gain : public Transformer {
    
public:
    
  Gain(Node& nd, const Amplitude& amp);
  int16_t next(int32_t tick, int32_t phi) override;

private:

  const Amplitude& amplitude;
  
};


class OneParFunc : public Transformer {

public:

  OneParFunc(Node& nd, float k);
  int16_t next(int32_t tick, int32_t phi) override;

protected:

  virtual float func(float k, float x) const = 0;
  
private:

  float constant;
  
};


class Compander : public OneParFunc {

public:

  Compander(Node& nd, float g);

private:
  
  float func(float k, float x) const override;
  
};


class Folder : public OneParFunc {

public:

  Folder(Node& nd, float g);

private:
  
  float func(float k, float x) const override;
  
};


class MeanFilter : public Transformer {

public:
    
  MeanFilter(Node& nd, int len);
  int16_t next(int32_t tick, int32_t phi) override;

private:

  std::unique_ptr<std::list<int32_t>> sums;
};


#endif
