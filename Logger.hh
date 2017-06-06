#pragma once

#include "BunchQueue.hh"
#include <fstream>
#include <string>
#include <condition_variable>
#include <mutex>
#include <thread>

/*
 * Has LogQueue, file to write to.
 */
namespace matan {

class Logger {
public:
  Logger(const std::string& ofname, bool wait = false) :
    m_logger(ofname),
    m_bWait(wait),
    m_worker([this]() { this->doit();}) {}
  ~Logger() {
    awake();
    m_bDone = true;
    m_worker.join(); //Perhaps I should detach?
  }
  void awake() { m_bWait = false; m_shouldWrite.notify_one(); }
  void sleep() { m_bWait = true; }
  const std::string& contents() { return m_buf; };
  void flush() { m_logger.write(m_buf); m_buf.clear(); }
  Logger& operator<<(const std::string& str) {
    m_buf += str;
    return *this;
  }
  Logger& operator<<(const char* c) {
    m_buf += c;
    return *this;
  }
  Logger& operator<<(char c) {
    m_buf += c;
    return *this;
  }
  Logger& operator<< (Logger& (*pf)(Logger&)) {
    pf(*this);
    return *this;
  }
  friend Logger& endlog(Logger& l);

private:
  class Logger_i {
    /*
     * Logger that attempts to minimize the time spent by the worker thread
     * that writes the log.
     */
  public:
    Logger_i(const std::string& ofname) : m_ofstream(ofname, std::ios::out) {}
    ~Logger_i() { flush(); m_ofstream.close(); }
    void write(std::string& line) { m_logs.push_back(line); }
    void flush() {
      for (const auto& line : m_logs.takeQueue()) {
        m_ofstream << line;
        m_ofstream.flush();
      }
    }

  private:
    BunchQueue<TakerQueue<std::string>> m_logs;
    std::ofstream m_ofstream;
  };

  void doit() {
    std::mutex mut;
    std::unique_lock<std::mutex> lock(mut);
    while (true) {
      if (m_bWait) {
        m_shouldWrite.wait(lock);
      }
      m_logger.flush();
      if (m_bDone) {
        break;
      }
    }
  }
  bool m_bDone = false;
  Logger_i m_logger;
  bool m_bWait;
  std::string m_buf;
  std::thread m_worker;
  std::condition_variable m_shouldWrite;
};

Logger& endlog(Logger& logger) {
  logger << '\n';
  logger.flush();
  return logger;
}

} // matan
