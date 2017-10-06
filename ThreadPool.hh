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
#include <future>

namespace matan {
  class ThreadPool {
  public:
    ThreadPool(int n = std::thread::hardware_concurrency());
    ~ThreadPool();
    int numThreads() const { return m_workers.size(); };
    template <typename F, typename ... Args> void push_back(F &&func, Args &&... args);
    
    template <typename F, typename ... Args> 
    std::future<typename std::result_of<F(Args...)>::type> push_back_get_future(F &&func, Args &&... args);
    
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
  
  template <typename F, typename ... Args>
  void ThreadPool::push_back(F &&func, Args &&... args) {
    /*
     * Args will be captured by value in the lambda. Please be
     * careful that any pointers passed to args cannot be invalidated
     * before the function is guaranteed to have run (waitFinished).
     */
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_tasks.emplace_back([&func, args...]() { func(args...); });
    m_cvTask.notify_one();
  }

  template <typename F, typename ... Args>
  std::future<typename std::result_of<F(Args...)>::type> ThreadPool::push_back_get_future(F &&func, Args &&... args) {
    typedef typename std::result_of<F(Args...)>::type RT;
    /*
     * Args will be captured by value in the lambda. Please be
     * careful that any pointers passed to args cannot be invalidated
     * before the function is guaranteed to have run (waitFinished, or future.get()).
     */
    std::promise<RT>* p = new std::promise<RT>();
    std::future<RT> fut = p->get_future();
    
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_tasks.emplace_back([p, &func, args...]() {
      p->set_value(func(args...));
      delete p; //A promise can be deleted after being set without invalidating its future
    });
    m_cvTask.notify_one();
    
    return fut;
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
