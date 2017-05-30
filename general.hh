#pragma once

#include <stdint.h>

#define likely(x)    __builtin_expect (!!(x), 1)
#define unlikely(x)  __builtin_expect (!!(x), 0)

namespace matan {

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

template <typename T, typename... Args>
inline void place(T* loc, Args&&... args) {
  ::new (loc) T(args...);
}

template <typename T, typename... Args>
inline void replace(T* p, Args&&... args) {
  p->~T();
  ::new (p) T(args...);
}

} // matan
