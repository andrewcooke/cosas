
#ifndef COSA_SOURCE_H
#define COSA_SOURCE_H

#include "constants.h"


// OLD TEXT BEFORE TICKS WERE SHIFTED:

// we measure time in ticks, one tick every 1/44100 s.

// the wavetable (unpacked to four quadrants) has a 44100 samples per
// wavelength.  to get a 440Hz tone we need to output the entire
// wavelength in 1/440 s.  since we output a sample every 1/44100 s we
// need to generate (1/440) / (1/44100) = 44100 / 440 =~ 100 samples.
// so we need to advance 44100 / 100 =~ 440 indices through the table
// on each tick.  an octave higher, at 880Hz, we need to output the
// entire wavelength in half that time so we need to advance twice as
// fast, advancing ~880 indices each tick.

// typically we talk about sine(omega t + phi); here we are doing
// wavetable[omega tick] where omega is the number of indices per
// tick.  for a 1Hz tone we need omega is 1, because we cover a whole
// cycle in 44100 ticks (when omega x tick = 44100).  in short, and
// exactly, omega is f.  this isn't by chance, it's because the
// wavetable has exactly the number of entries needed to "last" for
// one second.

// for fm modulation we need to evaluate sin(omega t + phi).  this
// translates into wavetable[f tick + phi] in our units.  note that
// phi is independent of f, so we it is not simply a scaling of
// time/tick.  so we need both tick and phi in our interface below.



// updated code: time is measured in 1/16 of a tick, so that we can
// have sub-Hz oscillatoes


class Source {

public:
  
  // tick needs to be signed because phi can be negative and we need
  // to add the two.  also, next is not const because of Latch.
  virtual int16_t next(int64_t tick, int32_t phi) = 0;
  int16_t next(int64_t tick);
  
};


int64_t tick2idx(int64_t tick);
int64_t hz2tick(float hz);


#endif
