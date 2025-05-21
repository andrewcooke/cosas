
#include <iostream>
#include <cstdint>
#include <cmath>
#include <memory>

#include "doctest.h"
#include "constants.h"
#include "maths.h"
#include "transformers.h"


Gain::Gain(Node& nd, const Amplitude amp)
  : Transformer(nd), amplitude(amp) {
  // std::cout << "set node " << &nd << std::endl;
};

int16_t Gain::next(int32_t tick, int32_t phi) {
  // std::cout << "calling next " << &node << std::endl;
  return amplitude.scale(node.next(tick, phi));
}


// these (float based) may be too slow?

OneParFunc::OneParFunc(Node& nd, float k)
  : Transformer(nd), constant(k) {};

int16_t OneParFunc::next(int32_t tick, int32_t phi) {
  int16_t sample = node.next(tick, phi);
  bool invert = sample < 0;
  float x = abs(sample) / (float)sample_max;
  float y = func(constant, x);
  sample = clip_16(sample_max * y);
  if (invert) sample = -sample;
  return sample;
}


Compander::Compander(Node& nd, float k)
  : OneParFunc(nd, k) {};

float Compander::func(float k, float x) const {
  return pow(x, k);
}


Folder::Folder(Node& nd, float k)
  : OneParFunc(nd, k) {};

// first half goes from flat to curve
// second half actually folds
float Folder::func(float k, float x) const {
  if (k < 1) return x * (1 + k * (1 - x));
  else return 1 - pow(k * x - 1, 2);
}


TEST_CASE("Folder") {

  Constant c1234 = Constant(1234);  // random +ve value
  Folder f0_12 = Folder(c1234, 0);
  CHECK(f0_12.next(0, 0) == 1234);
  Folder f1_12 = Folder(c1234, 1);
  CHECK(f1_12.next(0, 0) == 2421);  // not sure if correct, but more +ve
  Constant cm1234 = Constant(-1234);
  Folder f1_12x = Folder(cm1234, 1);
  CHECK(f1_12x.next(0, 0) == -2421);  // symmetrical
  
  Constant cmax = Constant(sample_max);  
  Folder f0_max = Folder(cmax, 0);
  CHECK(f0_max.next(0, 0) == sample_max);
  Folder f1_max = Folder(cmax, 1);
  CHECK(f1_max.next(0, 0) == sample_max);
  Folder f2_max = Folder(cmax, 2);
  CHECK(f2_max.next(0, 0) == 0);

}


MeanFilter::MeanFilter(Node& nd, int len)
  : Transformer(nd), sums(std::move(std::make_unique<std::vector<int32_t>>(len, 0))), i(0) {};

int16_t MeanFilter::next(int32_t tick, int32_t phi) {
  int32_t cur = node.next(tick, phi);
  for (int32_t& s : *sums) s += cur;
  int32_t next = (*sums)[i];
  (*sums)[i] = 0;
  i = (i + 1) % sums->size();
  return next / sums->size();
}

TEST_CASE("MeanFilter") {

  Sequence s = Sequence({0, 0, 100});
  MeanFilter mf = MeanFilter(s, 3);
  CHECK(mf.next(0, 0) == 0);
  CHECK(mf.next(0, 0) == 0);
  CHECK(mf.next(0, 0) == 33);
  CHECK(mf.next(0, 0) == 33);
  CHECK(mf.next(0, 0) == 33);
  CHECK(mf.next(0, 0) == 0);
  CHECK(mf.next(0, 0) == 0);

}
