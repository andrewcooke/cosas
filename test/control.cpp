
#include "doctest/doctest.h"

#include "cosas/control.h"


class Save : public Input {

public:

  Save(float *sav) : saved(sav) {};
  void set(float val) override {*saved = val;}
  
private:
  
  float *saved;
  
};
  

TEST_CASE("Control, Sigmoid") {

  float x;
  std::unique_ptr<Save> save = std::make_unique<Save>(&x);

  Sigmoid(*save, 1).set(0); CHECK(x == 0);
  Sigmoid(*save, 1).set(1); CHECK(x == 1);
  Sigmoid(*save, 1).set(0.5); CHECK(x == 0.5);
  Sigmoid(*save, 1).set(0.25); CHECK(x == 0.25);
  Sigmoid(*save, 1).set(0.75); CHECK(x == 0.75);

  // these are much closer to 0.5
  Sigmoid(*save, 0).set(0); CHECK(x == 0);
  Sigmoid(*save, 0).set(1); CHECK(x == 1);
  Sigmoid(*save, 0).set(0.5); CHECK(x == 0.5);
  Sigmoid(*save, 0).set(0.25); CHECK(x == 0.4375);
  Sigmoid(*save, 0).set(0.75); CHECK(x == 0.5625);
  
}


TEST_CASE("Control, Exp") {

  float x;
  std::unique_ptr<Save> save = std::make_unique<Save>(&x);
  
  Exp(*save).set(0); CHECK(x == doctest::Approx(0.1));
  Exp(*save).set(0.5); CHECK(x == doctest::Approx(1));
  Exp(*save).set(1); CHECK(x == doctest::Approx(10));

}
