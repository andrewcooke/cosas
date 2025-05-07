
import std;
using namespace std;

#include <cstdint>

#include "constants.h"
#include "model.h"


Balance::Balance(float wet) : wet(wet), wet_weight(wet * one), dry_weight((1 - wet) * one) {};

uint16_t Balance::combine(uint16_t wet, uint16_t dry) {
  return clip((wet_weight * wet + dry_weight * dry) >> one_bits);
}


Mixer::Mixer(Source src1, Source src2, AmpScale vol, Balance bal) : source1(src1), source2(src2), volume(vol), balance(bal);

Mixer::uint16_t next(int64_t tick, int32_t phi) const override {
  return volume.scale(balance.combine(source1.next(tick, phi), source2.next(tick, phi)));
}


class FMImpl : public Source {

public:

  FMImpl(Source car, Source mod) : carrier(car), modulator(mod);

  uint16_t next(int64_t tick, int32_t phi) const override {
    uint16_t mod = modulator.next(tick, phi);
    return carrier.next(tick, phi + ((int32_t)mod - sample_zero));
  };

private:

  Source carrier;
  Source modulator;
  
};
    
FM::FM(Source car, Source mod, AmpScale vol, Balance bal) : mixer(Mixer(car, FMImpl(car, mod), vol, bal));

uint16_t FM:next(int64_t tick, int32_t phi) {
  return mixer.next(tick, phi);
}


class AMImpl : public Source {

public:

  AMImpl(Source src1, Source src2) : source1(src1), source2(src2);

  uint16_t next(int64_t tick, int32_t phi) const override {
    int32_t s1 = source1.next(tick, phi);
    int32_t s2 = source2.next(tick, phi);
    return clip(((s1 - sample_zero) * (s2 - sample_zero)) / sample_zero + sample_zero);
  };

private:

  Source source1;
  Source source2;
  
};
    
AM::AM(Source src1, Source src2, AmpScale vol, Balance bal) : mixer(Mixer(car, AMImpl(scr1, src2), vol, bal));

uint16_t AM:next(int64_t tick, int32_t phi) {
  return mixer.next(tick, phi);
}
