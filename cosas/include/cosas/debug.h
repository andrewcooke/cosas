
#ifndef COSAS_DEBUG_H
#define COSAS_DEBUG_H


#include <iostream>


class BaseDebug {

protected:

  static bool used;
  BaseDebug() = default;

public:

  virtual void init();

  static void log(std::ostream& os) {
    os << std::endl;;
  }

  template <typename T, typename... Args>
  static void log(std::ostream& os, const T& firstArg, const Args&... restArgs) {
    if (used) {
      os << firstArg << " ";
      log(os, restArgs...);
    }
  }

  template <typename... Args>
  static void log(const Args&... restArgs) {if (used) log(std::cout, restArgs...);}

};


#endif
