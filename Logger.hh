#pragma once

#include "BunchQueue.hh"
#include <fstream>
#include <condition_variable>
#include <mutex>
#include <thread>

/*
 * Has LogQueue, file to write to.
 */
namespace matan {

template <typename Log>
class Logger_i {
  /*
   * Logger that attempts to minimize the time spent by the worker thread
   * that writes the log.
   */
public:
  Logger_i(const std::string& ofname) : m_fstream(ofname, std::ios::out) {
    if (!m_fstream.is_open()) {
    }
  }
  ~Logger_i() { flush(); m_fstream.close(); }
  void write(Log& line) { m_logs.push_back(line); }
  void flush() {
    for (const Log& line : m_logs.takeQueue()) {
      m_fstream << line << std::endl; // should I not put endl?
      m_fstream.flush();
    }
  }

private:
  BunchQueue<TakerQueue<Log>> m_logs;
  std::fstream m_fstream;
};

template <typename Log>
class Logger {
public:
  Logger(const std::string& ofname, bool wait = false) :
    m_pLogger(new Logger_i<Log>(ofname)),
    m_bWait(wait),
    m_worker([this]() {
               std::mutex mut;
               std::unique_lock<std::mutex> lock(mut);
               while (true) {
                 if (this->m_bWait) {
                   this->m_shouldWrite.wait(lock);
                 }
                 this->m_pLogger->flush();
                 if (this->m_bDone) {
                   break;
                 }
               }
             }) {}
  ~Logger() {
    awake();
    m_bDone = true;
    m_worker.join(); //Perhaps I should detach...
  }
  void awake() { m_bWait = false; m_shouldWrite.notify_one(); }
  void sleep() { m_bWait = true; }
  void write(Log& line) { m_pLogger->write(line); }
private:
  bool m_bDone = false;
  std::unique_ptr<Logger_i<Log>> m_pLogger;
  bool m_bWait;
  std::thread m_worker;
  std::condition_variable m_shouldWrite;
};

} // matan
