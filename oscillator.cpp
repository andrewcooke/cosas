
import std;
using namespace std;

#include "constants.h"
#include "oscillator.h"


StepScale::StepScale(uint32_t n, uint32_t d) : numerator(n), denominator(d) {
    three = (d % 3) == 0;
    if (three) d /= 3;
    bits = 0;
    while (d > 1) {
        if (d & 1) throw invalid_argument("denominator should be 2^n x [3,1]");
        bits++; d >>= 1;
    }
};

uint64_t StepScale::scale(uint64_t tick) {
    uint64_t t = tick * numerator;
    if (three) t /= 3;  // https://stackoverflow.com/a/171369
    return t >> bits;
}


Wavetable::Wavetable() : quarter_table() {
    for (int i = 0; i < table_size; i++) {
        quarter_table[i] = (zero_bits - 1) * sin(2 * numbers::pi * i / sample_rate);
    }
};

uint16_t Wavetable::at_uint16_t(uint64_t tick, StepScale scale) {
    uint16_t index = (uint16_t)(scale.scale(tick) % sample_rate);
    if (index < table_size) return quarter_table[index % table_size] + zero_bits;
    else if (index < 2 * table_size) return quarter_table[table_size - 1 - (index % table_size)] + zero_bits;
    else if (index < 3 * table_size) return zero_bits - quarter_table[index % table_size];
    else return zero_bits - quarter_table[table_size - 1 - (index % table_size)];
}

float Wavetable::at_float(uint64_t tick, StepScale scale) {
    return ((at_uint16_t(tick, scale) - zero_bits) / (float)zero_bits);
}
