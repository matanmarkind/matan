/*
 * credit: WhozCraig on Stackoverflow
 * http://stackoverflow.com/questions/23896421/efficiently-waiting-for-all-m_tasks-in-a-threadpool-to-finish
 *
 * DOESN'T WORK :'(
 */

#include <deque>
#include <functional>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <iostream>

namespace matan {
  class ThreadPool {
  public:
    ThreadPool(const unsigned int n = std::thread::hardware_concurrency());
    ThreadPool(const ThreadPool& tp);
    template<class F, class... Args> void enqueue(F &&f, Args&&... args);
    void waitFinished();
    ~ThreadPool();
    unsigned int getProcessed() const { return m_processed; }

  private:
    std::vector<std::thread> m_workers;
    std::deque<std::function<void()>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_cvTask;
    std::condition_variable m_cvFinished;
    std::atomic_uint m_busy, m_processed;
    bool stop;

    void threadProc();
  };

  ThreadPool::ThreadPool(unsigned int n) :
          m_busy(0), m_processed(0), stop(false) {
    for (unsigned int i = 0; i < n; ++i)
      m_workers.emplace_back(std::bind(&ThreadPool::threadProc, this));
  }

  ThreadPool::ThreadPool(const ThreadPool& tp) {
    ThreadPool(tp.m_workers.capacity());
  }

  ThreadPool::~ThreadPool() {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    stop = true;
    m_cvTask.notify_all();
    lock.unlock();

    for (auto &t : m_workers)
      t.join();
  }

  void ThreadPool::threadProc() {
    while (true) {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_cvTask.wait(lock, [this]() { return stop || !m_tasks.empty(); });
      if (!m_tasks.empty()) {
        ++m_busy;
        auto fn = m_tasks.front();
        m_tasks.pop_front();
        lock.unlock(); //stop blocking, run async

        fn(); //run the function

        ++m_processed;
        --m_busy;
        m_cvFinished.notify_one();
      } else if (stop) {
        break;
      }
    }
  }

  template<class F, class... Args>
  void ThreadPool::enqueue(F&& f, Args&&... args) {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_tasks.emplace_back(std::bind(f, args...));
    m_cvTask.notify_one();
  }

  void ThreadPool::waitFinished() {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_cvFinished.wait(lock,
                      [this]() {
                        return m_tasks.empty() && (m_busy == 0);
                      });
  }
} //namespace matan