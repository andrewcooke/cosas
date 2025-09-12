
#ifndef COSAS_MODULATORS_H
#define COSAS_MODULATORS_H

#include "cosas/node.h"


// constructor takes two nodes

class Modulator : public RelSource {};


class FM : public Modulator {
public:
  FM(RelSource& car, RelSource& mod);
  [[nodiscard]] int16_t next(int32_t phi) override;
private:
  RelSource& carrier;
  RelSource& modulator;
};


class AM : public Modulator {
public:
  // note that this is not symmetric - nd1 is mixed against the ring mod output
  AM(RelSource& src1, RelSource& src2);
  [[nodiscard]] int16_t next(int32_t phi) override;
private:
  RelSource& src1;
  RelSource& src2;
};


#endif
