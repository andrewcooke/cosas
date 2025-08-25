
#include "cosas/params.h"


Blank::Blank() : Param(0, 0, false, 0, 0), delegate(nullptr) {};

void Blank::set(float value) {
  if (delegate != nullptr) {
    delegate->set(value);
  }
}

void Blank::unblank(Param *del) {
  this->delegate = del;
  scale = del->scale;
  linearity = del->linearity;
  log = del->log;
  lo = del->lo;
  hi = del->hi;
}




