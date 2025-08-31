
#ifndef COSAS_PATOMIC_H
#define COSAS_PATOMIC_H


#ifdef PICO
#include "RP2040Atomic.hpp"
#define ATOMIC(type) patom::types::patomic_##type
#define LOAD(var) var.load()
#else
#define ATOMIC(type) type
#define LOAD(var) var
#endif


#endif

