#ifndef RAPUNZEL_UTIL_H
#define RAPUNZEL_UTIL_H

#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <memory>
#include <utility>
#include "rapunzel_util.h"


/** workaround for gcc 4.8 which doesn't have make_unique */
#ifdef MAKE_UNIQUE
namespace std {
  template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args)
  {
      return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }
}
#endif

#endif