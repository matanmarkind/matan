#pragma once

#include "BunchQueue.hh"
#include <condition_variable>
#include <thread>

namespace matan {

class AsyncWorker {
  /*
   * A class to allow for a process to contain a worker thread that follows
   * a simple loop. The expected usage is for there to be some sort of queue
   * like data structure that the owners write to, and the worker thread
   * will run over each element performing some operation defined by doit.
   *
   * This class is capable of Multi writer, single reader (the internal thread).
   * but the details of implementation will determine the reality of if you
   * can take multiple writers.
   */
public:
  AsyncWorker() : m_worker([this]() { this->workerLoop();}) {}
  virtual ~AsyncWorker() = 0;

protected:
  void workerLoop() {
    std::unique_lock<std::mutex> lock(m_mtx);
    lock.unlock();
    while (true) {
      lock.lock();
      if (shouldSleep()) {
        m_shouldDoit.wait(lock);
      }
      lock.unlock();
      doit();
      if (m_bDone) {
        break;
      }
      doit();
    }
  }
  /*
   * doit is the function that we actual want the worker thread to perform.
   * I assume that each doit call is enough to completely utilize all the
   * contents on the worker threads "queue."
   */
  virtual void doit() = 0;
  /*
   * Checks if there is any work for the worker thread to do, and if not puts
   * the thread to sleep.
   */
  virtual bool shouldSleep() = 0;
  /*
   * Need to prevent this situation (in this order) so as not to miss a
   * notify when deciding to wait.
   *
   * flush_thread: m_logs.empty() == true
   * write_thread: m_logs.push_back(m_buf)
   * write_thread: m_shouldWrite.notify_one()
   * flush_thread: m_shouldWrite.wait(lock)
   */
  void notifyNewEle() {
    m_mtx.lock();
    m_shouldDoit.notify_one();
    m_mtx.unlock();
  }

  bool m_bDone = false;
  std::thread m_worker;

private:
  std::mutex m_mtx;
  std::condition_variable m_shouldDoit;
};

inline AsyncWorker::~AsyncWorker() {}

} // matan

