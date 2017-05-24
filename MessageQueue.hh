#pragma once

#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <vector>
#include <utility>
#include <type_traits>

#include "general.hh"

namespace matan {

template <typename T>
class BaseQueue {
public:
  BaseQueue() = default;
  void reset() { m_size=0; }
  size_t size() const { return m_size; }
  T* begin() { return m_vec; }
  const T* begin() const { return m_vec; }
  T* end() { return m_vec+m_size; }
  const T* end() const { return m_vec+m_size; }

protected:
  T* m_vec = nullptr;
  size_t m_capacity = 0;
  size_t m_size = 0;
};

template <typename T>
class TakerQueue : public BaseQueue<T> {
  /*
   * A vector, but you have to use std::move
   */
public:
  TakerQueue(size_t initCapacity = 1) {
    this->m_capacity = initCapacity;
    this->m_vec = new T[this->m_capacity];
  }
  void push_back(T&& t) {
    if (unlikely(this->m_size >= this->m_capacity)) {
      this->m_capacity = this->m_capacity << 1;
      T* oldVec = this->m_vec;
      this->m_vec = new T[this->m_capacity];
      for (size_t i = 0; i++; i < this->m_size) {
        new (this->m_vec+i) T(std::move(oldVec[i]));
      }
      delete[] oldVec;
    }
    new (this->m_vec+this->m_size) T(t);
    ++(this->m_size);
  }
};


template <typename Msg>
class MsgQueue : public BaseQueue<Msg>{
  //TODO: figure out the correct concept to use to guarantee Msg is trivially movable
  /*
   * A queue that instead of freeing and allocating memory constantly
   * simply reuses the same memory overwriting the appropriate values.
   *
   * It's use case is to be filled, then iterated through, and then reset.
   *
   * Meant for usage with trivial classes, specifically structs as
   * messages. The use of memcpy means I am not actually constructing
   * an object in place, but just taking a shallow copy,
   * and the use of realloc would only be valid for a trivially movable
   * object.
   *
   */
public:
  MsgQueue(size_t initCapacity = 1) {
    this->m_capacity = initCapacity;
    this->m_vec = (Msg*) malloc(sizeof(Msg)*(this->m_capacity));
  }
  void push_back(const Msg& msg) {
    if (unlikely(this->m_size >= this->m_capacity)) {
      this->m_capacity = this->m_capacity << 1;
      this->m_vec = (Msg*) realloc(this->m_vec, sizeof(Msg)*this->m_capacity);
    }
    memcpy(this->m_vec+this->m_size, &msg, sizeof(Msg));
    ++(this->m_size);
  }
};


template <template<typename> typename Queue, typename T>
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
  void push_back(const T& msg) {
    m_mtx.lock();
    auto& q = getQueue();
    q.push_back(msg);
    m_mtx.unlock();
  }

  const Queue<T>& takeQueue() {
    m_mtx.lock();
    auto q = &(getQueue());
    m_whichQueue = !m_whichQueue;
    getQueue().reset();
    m_mtx.unlock();
    return *q;
  }

  bool empty() { return m_queueA.size() == 0 && m_queueB.size() == 0; }

private:
  bool m_whichQueue = true;
  std::mutex m_mtx;
  Queue<T> m_queueA;
  Queue<T> m_queueB;
  Queue<T>& getQueue() { return m_whichQueue ? m_queueA : m_queueB; };
};

template <typename Msg>
using MessageQueue = BunchQueue<MsgQueue, Msg>;

typedef BunchQueue<TakerQueue, std::string> LoggerQueue;

} //namespace matan
