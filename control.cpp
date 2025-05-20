
#include <cmath>

#include "doctest.h"

#include "control.h"


Delegate::Delegate(std::unique_ptr<Input> del) : delegate(std::move(del)) {};


Change::Change(std::unique_ptr<Input> del) : Delegate(std::move(del)) {};

void Change::set(float v) {
  if (v != prev) {
    delegate->set(v);
    prev = v;
  }
}


Sigmoid::Sigmoid(std::unique_ptr<Input> del, float lin)
  : Delegate(std::move(del)), linear(lin) {};

void Sigmoid::set(float v) {
  float v2 = v - 0.5;
  // https://www.desmos.com/calculator/rbmvnpsmrm
  delegate->set(4 * (1 - linear) * pow(v2, 3) + linear * v2 + 0.5);
}


class Save : public Input {

public:

  Save(float *sav) : saved(sav) {};
  void set(float val) override {*saved = val;}
  
private:
  
  float *saved;
  
};
  

TEST_CASE("Sigmoid") {

  float x;

  Sigmoid(std::make_unique<Save>(&x), 1).set(0); CHECK(x == 0);
  Sigmoid(std::make_unique<Save>(&x), 1).set(1); CHECK(x == 1);
  Sigmoid(std::make_unique<Save>(&x), 1).set(0.5); CHECK(x == 0.5);
  Sigmoid(std::make_unique<Save>(&x), 1).set(0.25); CHECK(x == 0.25);
  Sigmoid(std::make_unique<Save>(&x), 1).set(0.75); CHECK(x == 0.75);

  // these are much closer to 0.5
  Sigmoid(std::make_unique<Save>(&x), 0).set(0); CHECK(x == 0);
  Sigmoid(std::make_unique<Save>(&x), 0).set(1); CHECK(x == 1);
  Sigmoid(std::make_unique<Save>(&x), 0).set(0.5); CHECK(x == 0.5);
  Sigmoid(std::make_unique<Save>(&x), 0).set(0.25); CHECK(x == 0.4375);
  Sigmoid(std::make_unique<Save>(&x), 0).set(0.75); CHECK(x == 0.5625);
  
}


Exp::Exp(std::unique_ptr<Input> del) : Delegate(std::move(del)) {};

void Exp::set(float v) {
  delegate->set(pow(10, 2 * (v - 0.5)));
};


TEST_CASE("Exp") {

  float x;
  
  Exp(std::make_unique<Save>(&x)).set(0); CHECK(x == doctest::Approx(0.1));
  Exp(std::make_unique<Save>(&x)).set(0.5); CHECK(x == doctest::Approx(1));
  Exp(std::make_unique<Save>(&x)).set(1); CHECK(x == doctest::Approx(10));

};


