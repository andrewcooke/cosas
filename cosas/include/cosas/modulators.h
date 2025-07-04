
#ifndef COSA_MODULATORS_H
#define COSA_MODULATORS_H

#include "cosas/node.h"


// constructor takes two nodes

class Modulator : public RelSource {};


class FM : public Modulator {
public:
  FM(const RelSource& car, const RelSource& mod);
  [[nodiscard]] int16_t next(int32_t delta, int32_t phi) const override;
private:
  const RelSource& carrier;
  const RelSource& modulator;
};


class AM : public Modulator {
public:
  // note that this is not symmetric - nd1 is mixed against the ring mod output
  AM(const RelSource& src1, const RelSource& src2);
  [[nodiscard]] int16_t next(int32_t tick, int32_t phi) const override;
private:
  const RelSource& src1;
  const RelSource& src2;
};


#endif
