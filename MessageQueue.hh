#pragma once

#include <stdlib.h>
#include <string.h>
#include <mutex>

#include "general.hh"

namespace matan {
template <typename Msg>
class BunchQueue {
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
  BunchQueue(size_t initCapacity = 1) :
      m_capacity(initCapacity) {
    m_vec = (Msg*) malloc(sizeof(Msg)*m_capacity);
  }
  void reset() { m_size=0; }
  size_t size() const { return m_size; }
  Msg* begin() { return m_vec; }
  const Msg* begin() const { return m_vec; }
  Msg* end() { return m_vec+m_size; }
  const Msg* end() const { return m_vec+m_size; }
  void push_back(const Msg& msg) {
    if (unlikely(m_size >= m_capacity)) {
      m_capacity = m_capacity << 1;
      m_vec = (Msg*) realloc(m_vec, sizeof(Msg)*m_capacity);
    }
    memcpy(m_vec+m_size, &msg, sizeof(Msg));
    ++m_size;
  }

private:
  Msg* m_vec;
  size_t m_capacity;
  size_t m_size = 0;
};


template <typename Msg>
class MessageQueue {
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
  MessageQueue(size_t initCapacity = 1) :
    m_queueA(initCapacity), m_queueB(initCapacity) {
  }
  void push_back(const Msg& msg) {
    m_mtx.lock();
    auto& q = getQueue();
    q.push_back(msg);
    m_mtx.unlock();
  }

  const BunchQueue<Msg>& takeQueue() {
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
  BunchQueue<Msg> m_queueA;
  BunchQueue<Msg> m_queueB;
  BunchQueue<Msg>& getQueue() { return m_whichQueue ? m_queueA : m_queueB; };
};
} //namespace matan
