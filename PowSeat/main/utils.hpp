#pragma once
#include <algorithm>

/** Clamp v into [lo, hi]. Works for any comparable type T. */
template<typename T>
static inline T clamp(T v, T lo, T hi){
  return std::max(lo, std::min(v, hi));
}
