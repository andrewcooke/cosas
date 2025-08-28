
#include "cosas/params.h"


Blank::Blank() : delegate(nullptr) {};

void Blank::set(float value) {
  if (delegate != nullptr) {
    delegate->set(value);
  }
}

float Blank::get() {
  if (delegate != nullptr) {
    return delegate->get();
  }
  return 0.0;
}

void Blank::unblank(Param *del) {
  this->delegate = del;
  scale = del->scale;
  linearity = del->linearity;
  log = del->log;
  lo = del->lo;
  hi = del->hi;
}




