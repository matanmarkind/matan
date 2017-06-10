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

class BunchLogger {
public:
  BunchLogger(const std::string& ofname, bool wait = false);
  ~BunchLogger();
  void awake() { m_bWait = false; m_shouldWrite.notify_one(); }
  void sleep() { m_bWait = true; }
  const std::string& contents() { return m_buf; };
  void flush() { m_logs.push_back(m_buf); m_buf.clear(); }
  BunchLogger& operator<<(const std::string& str) {m_buf += str; return *this;}
  BunchLogger& operator<<(const char* c) { m_buf += c; return *this; }
  BunchLogger& operator<<(char c) { m_buf += c; return *this; }
  BunchLogger& operator<<(BunchLogger& (*pf)(BunchLogger&)) {pf(*this); return *this;}

private:
  void doit();
  void trueFlush();

  bool m_bDone = false;
  bool m_bWait;
  std::string m_buf;
  BunchQueue<TakerQueue<std::string>> m_logs;
  std::ofstream m_ofstream;
  std::thread m_worker;
  std::condition_variable m_shouldWrite;
};

} // matan

namespace std {
inline matan::BunchLogger& endl(matan::BunchLogger& logger) {
  logger << '\n';
  logger.flush();
  return logger;
}
}
