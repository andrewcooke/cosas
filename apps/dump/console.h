
#ifndef COSAS_CONSOLE_H
#define COSAS_CONSOLE_H

#include "cosas/engine_old.h"
#include "cosas/source.h"

void dump_w_top(OldManager::OldEngine e, size_t n, size_t p);
void dump_w_gain(OldManager::OldEngine e, size_t n);
void dump_w_wdex(OldManager::OldEngine e, size_t n);
void dump_poly(float f, size_t shp, size_t asym, size_t off, size_t n);
void dump_dex(float f, Wavelib& w, size_t idx);
void dump(OldManager::OldEngine e, size_t n);


#endif
