
#ifndef COSAS_COMMON_H
#define COSAS_COMMON_H


#include <cstddef>  // size_t


enum When {Now, Prev};
static constexpr size_t N_WHEN = Prev + 1;

// this matches the first 3/4 of Codec::Ctrl
enum Knob {Main, X, Y};
static constexpr size_t N_KNOBS = Y + 1;


#endif

