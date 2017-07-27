#pragma once

#include "BunchQueue.hh"
#include <thread>
#include <atomic>

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
  AsyncWorker() : m_worker(new std::thread([this]() { this->workerLoop();})) {}
  virtual ~AsyncWorker() = 0;
  AsyncWorker(const AsyncWorker&) = delete;

protected:
  void workerLoop() {
      while (true) {
        waitTillNeeded();
        doit();
        if (unlikely(m_bDone)) {
          break;
        }
      }
      doit();
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
  virtual bool shouldSleep() const = 0;

  /*
   * Locked so that waitTillNeeded can't be in an indeterminate state. Either
   * set beforehand so never wait, or set after already waiting so that the
   * notify that follows won't be waster in between the boolean and the
   * actual call to wait.
   */
  void done() {
    std::unique_lock<std::mutex> lock(m_mtx);
    m_bDone = true;
    notifyWorker();
    lock.unlock();
    m_worker->join();
  }
  
  void notifyWorker() {
    m_bRealWakeup = true;
    m_shouldDoit.notify_one();
  }

  std::atomic_bool m_bDone{false};
  std::atomic_bool m_bRealWakeup{false};
  std::unique_ptr<std::thread> m_worker;
  std::condition_variable m_shouldDoit;

private:
  void waitTillNeeded() {
    try {
      shouldSleep();
    } catch (const std::system_error& e) {
      std::cout << "error shouldSleep" << std::endl;
    }
    std::unique_lock<std::mutex> lock(m_mtx);
    if (!m_bDone && shouldSleep()) {
      m_bRealWakeup = false;
      m_shouldDoit.wait(lock, [this] { return this->realWakeup(); });
    }
  }

  //Prevent spurious system wake up calls
  bool realWakeup() { return m_bRealWakeup; }

  std::mutex m_mtx;
};

inline AsyncWorker::~AsyncWorker() {}

} // matan
