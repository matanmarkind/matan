#pragma once

#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <vector>
#include <utility>
#include <condition_variable>
#include <type_traits>
#include <assert.h>
#include "general.hh"

#include <iostream>  //PUSH_ASSERT



namespace matan {

#define BOUNDS_CHECK 1
template <typename T>
class VecQueue {
public:
  typedef T value_type;
  VecQueue(size_t initCapacity=1) : m_capacity(initCapacity), m_size(0) {
    m_vec = (T*) malloc(sizeof(T)*(m_capacity));
  }
  VecQueue(VecQueue& vq) {
    m_capacity = vq.m_capacity;
    m_size = 0;
    m_vec = (T*) malloc(sizeof(T)*(m_capacity));
    for (size_t i = 0; i < vq.m_size; i++) {
      if (std::is_trivially_move_constructible<T>::value) {
        m_size = vq.m_size;
        memcpy(m_vec, vq.m_vec, m_size);
      } else {
        new (m_vec+i) T(vq.m_vec[i]);
      }
    }
  }
  VecQueue(VecQueue&& vq) {
    m_capacity = vq.m_capacity;
    vq.m_capacity = 0;
    m_size = vq.m_size;
    vq.m_size = 0;
    m_vec = vq.m_vec;
    vq.m_vec = nullptr;
  }
  ~VecQueue() { 
    clearContents();
  }

  void reset() {
    if (!std::is_trivially_destructible<T>::value) {
      for (size_t i = 0; i < m_size; i++) {
        m_vec[i].~T();
      }
    }
    m_size=0;
  }
  void clear() { m_size=0; m_capacity=1; clearContents(); m_vec = malloc(sizeof(T)); }
  size_t size() const { return m_size; }
  T* begin() { return m_vec; }
  const T* begin() const { return m_vec; }
  T* end() { return m_vec+m_size; }
  const T* end() const { return m_vec+m_size; }
  T& operator[](size_t i) {
    if (BOUNDS_CHECK) {
      assert(i < m_size);
    }
    return m_vec[i];
  }

  void push_back(const T& t) {
    capacity_check();
    new (m_vec+m_size) T(t);
    ++(m_size);
  }
  void push_back(T&& t) {
    capacity_check();
    new (m_vec+m_size) T(std::move(t));
    ++(m_size);
  }
  template <typename ...Args>
  void emplace_back(Args... args) {
    capacity_check();
    new (m_vec+m_size) T(args...);
    ++(m_size);
  }


private:
  void capacity_check() {
    if (std::is_trivially_move_constructible<T>::value && 
        unlikely(m_size >= m_capacity)) {
      m_capacity = m_capacity << 1;
      m_vec = (T*) realloc(m_vec, sizeof(T)*m_capacity);
    } else if (unlikely(m_size >= m_capacity)) {
      m_capacity = m_capacity << 1;
      T* oldVec = m_vec;
      m_vec = (T*) malloc(sizeof(T)*(m_capacity));
      for (size_t i = 0; i < m_size; i++) {
        if (std::is_move_constructible<T>::value) {
          new (m_vec+i) T(std::move(oldVec[i]));
        } else if (std::is_copy_constructible<T>::value) {
          new (m_vec+i) T(oldVec[i]);
        }
      }
      if (std::is_move_constructible<T>::value) {
        free(oldVec); // Since I move all the elements out I can't delete
      } else if (std::is_copy_constructible<T>::value) {
        delete[] oldVec;
      }
    }
  }

  void clearContents() {
    if (!std::is_trivially_destructible<T>::value) {
      for (size_t i = 0; i < m_size; i++) {
        m_vec[i].~T();
      }
    }
    free(m_vec);
  }

  size_t m_capacity;
  size_t m_size;
  T* m_vec;
  static_assert(std::is_move_constructible<T>::value || std::is_copy_constructible<T>::value,
                "VecQueue contents must either be move or copy constructible");
};
#undef BOUNDS_CHECK


template <typename T>
class BunchQueue {
  /*
   * Multi writer single reader.
   *
   * Instead of popping off individual messages the reader takes all of them
   * at once. This works well if the architecture is focused on bunches.
   * Also good memory wise because it means fewer allocations and frees and
   * allows for adjacent memory access when reading through the messages.
   * Drawback is that you have a relatively large memory footprint with 1
   * vector just sitting around. Works best if you are not RAM bound and can
   * expect fairly consistent bunch sizes.
   */
public:
  BunchQueue(size_t initCapacity = 1) :
    m_queueA(initCapacity), m_queueB(initCapacity) {
  }
  void push_back(const T& t) {
    std::unique_lock<std::mutex> lock(m_mtx);
    auto& q = getQueue();
    q.push_back(t);
  }
  void push_back(T&& t) {
    std::unique_lock<std::mutex> lock(m_mtx);
    auto& q = getQueue();
    q.push_back(t);
  }

  const VecQueue<T>& takeQueue() {
    std::unique_lock<std::mutex> lock(m_mtx);
    auto q = &(getQueue());
    m_whichQueue = !m_whichQueue;
    getQueue().reset();
    return *q;
  }

  bool empty() const {
    std::unique_lock<std::mutex> lock(m_mtx);
    return m_queueA.size() == 0 && m_queueB.size() == 0;
  }

private:
  bool m_whichQueue = true;
  mutable std::mutex m_mtx;
  VecQueue<T> m_queueA;
  VecQueue<T> m_queueB;
  VecQueue<T>& getQueue() {
    //Must be called from a locked scope
    return m_whichQueue ? m_queueA : m_queueB;
  }
};

} //namespace matan
