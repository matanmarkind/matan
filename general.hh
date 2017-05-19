#define likely(x)    __builtin_expect (!!(x), 1)
#define unlikely(x)  __builtin_expect (!!(x), 0)

namespace matan {
  template <typename T, typename... Args>
  inline void place(T* loc, Args&&... args) {
    ::new (loc) T(args...);
  }

  template <typename T, typename... Args>
  inline void replace(T* p, Args&&... args) {
    p->~T();
    ::new (p) T(args...);
  }
}
