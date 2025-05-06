
#ifndef FMCOSA_OSCILLATOR_H
#define FMCOSA_OSCILLATOR_H

import std;
using namespace std;

#include "constants.h"


class StepScale {

public:

    const uint32_t numerator;
    const uint32_t denominator;

    StepScale(uint32_t n, uint32_t d);
    uint64_t scale(uint64_t tick);

private:

    uint8_t bits;
    bool three;

};

const auto unit_scale = StepScale(1, 1);


class Wavetable {

public:

    Wavetable();
    uint16_t at_uint16_t(uint64_t tick, StepScale scale);
    float at_float(uint64_t tick, StepScale scale);

private:

    array<uint16_t, table_size> quarter_table;

};

#endif