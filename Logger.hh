#pragma once

#include "BunchQueue.hh"
#include <fstream>
#include <string>
#include <condition_variable>
#include <thread>
#include <atomic>

/*
 * Has LogQueue, file to write to.
 */
namespace matan {

class Logger {
public:
  Logger(const std::string& ofname);
  ~Logger();
  Logger& operator<<(const std::string& str) {m_buf += str; return *this;}
  Logger& operator<<(const char* c) { m_buf += c; return *this; }
  Logger& operator<<(char c) { m_buf += c; return *this; }
  Logger& operator<<(Logger& (*pf)(Logger&)) {return pf(*this);}
  void flush();
  void finish();

private:
  void doFlush();
  void doit();
  void trueFlush();

  bool m_bDone = false;
  std::string m_buf;
  BunchQueue<TakerQueue<std::string>> m_logs;
  std::ofstream m_ofstream;
  std::thread m_worker;
  std::mutex m_mtx;
  std::condition_variable m_shouldWrite;
  static constexpr std::size_t MAX_LEN = 3 * 1024;
};

} // matan

namespace std {

inline matan::Logger& endl(matan::Logger& logger) {
  logger << '\n';
  logger.flush();
  return logger;
}

}
