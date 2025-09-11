
#ifndef COSAS_MODULATORS_H
#define COSAS_MODULATORS_H

#include "cosas/node.h"


// constructor takes two nodes

class Modulator : public PhaseSource {};


class FM : public Modulator {
public:
  FM(PhaseSource& car, PhaseSource& mod);
  [[nodiscard]] int16_t next(uint32_t tick, int32_t phi) override;
private:
  PhaseSource& carrier;
  PhaseSource& modulator;
};


class AM : public Modulator {
public:
  // note that this is not symmetric - nd1 is mixed against the ring mod output
  AM(PhaseSource& src1, PhaseSource& src2);
  [[nodiscard]] int16_t next(uint32_t tick, int32_t phi) override;
private:
  PhaseSource& src1;
  PhaseSource& src2;
};


#endif
