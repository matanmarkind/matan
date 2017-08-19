/*
 * credit: WhozCraig on Stackoverflow
 * http://stackoverflow.com/questions/23896421/efficiently-waiting-for-all-m_tasks-in-a-threadpool-to-finish
 */
#pragma once

#include <deque>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <iostream>
#include <vector>

namespace matan {
  class ThreadPool {
  public:
    ThreadPool(int n = std::thread::hardware_concurrency());
    ~ThreadPool();
    int numThreads() const { return m_workers.size(); };
    template <class F> void push_back(F &&func);
    void waitFinished();

  private:
    std::vector<std::thread> m_workers;
    std::deque<std::function<void()>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_cvTask;
    std::condition_variable m_cvFinished;
    int m_busy = 0;
    bool m_bStop = false;

    void threadProc();
  };

  ThreadPool::ThreadPool(int n) {
    for (int i = 0; i < n; ++i) {
      m_workers.emplace_back([this](){this->threadProc();});
    }
  }

  ThreadPool::~ThreadPool() {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_bStop = true;
    m_cvTask.notify_all();
    lock.unlock();

    for (auto &t : m_workers) {
      t.join();
    }
  }
  
  template <class F>
  void ThreadPool::push_back(F &&func) {
    /*
     * The variables passed/captured within func are probably
     * the biggest opportunity for an issue if passed by reference.
     * If you have a vector<SomeClass> and for each object push
     * a function with a reference to that object onto the threadpool
     * and then before waitFinished you push_back a new object onto
     * the vector you could invalidate all of the references.
     */
    std::unique_lock<std::mutex> lock(m_queueMutex);
    /* 
     * Is there a way for me not to have to wrap this inside of a
     * lambda? Can I just not care about the return type of
     * the function?
     */
    m_tasks.emplace_back([&func]() { func(); });
    m_cvTask.notify_one();
  }

  void ThreadPool::waitFinished() {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_cvFinished.wait(lock,
                      [this]() {
                        return m_tasks.empty() && (m_busy == 0);
                      });
    m_busy = 0;
  }

  void ThreadPool::threadProc() {
    while (true) {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_cvTask.wait(lock, [this]() { return m_bStop || !m_tasks.empty(); });
      if (!m_tasks.empty()) {
        ++m_busy;
        auto fn = m_tasks.front();
        m_tasks.pop_front();
        lock.unlock();

        fn(); //run the function without blocking the other threads

        lock.lock();
        /*
         * Need to lock this section so that don't make 'final' call of
         * m_cvFinished.notify_one(), while another one is previously running.
         * This will cause the notify to go unnoticed, beacause we won't be
         * waiting at this point, and then will never again be notified.
         */
        --m_busy;
        m_cvFinished.notify_one();
        lock.unlock();
      } else if (m_bStop) {
        break;
      }
    }
  }

} //namespace matan
