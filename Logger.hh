#pragma once

#include "BunchQueue.hh"
#include <fstream>
#include <string>
#include <condition_variable>
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
  BunchLogger& operator<<(const std::string& str) {m_buf += str; return *this;}
  BunchLogger& operator<<(const char* c) { m_buf += c; return *this; }
  BunchLogger& operator<<(char c) { m_buf += c; return *this; }
  BunchLogger& operator<<(BunchLogger& (*pf)(BunchLogger&)) {return pf(*this);}
  void flush();

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


class BufferLogger {
public:
  // Single writer single reader
  BufferLogger(const std::string& ofname);
  ~BufferLogger();
  /*
   * When writing to the string I don't need to lock since this is single
   * write single reader and the reader gets called from the writing thread. So
   * I can guarantee the we won't be trying to flush and << at the same time
   * since if there is only 1 writing thread it can't call multiple <<
   * at the same time.
   */
  BufferLogger& operator<<(const std::string& str) {getString() += str; return *this;}
  BufferLogger& operator<<(const char* c) { getString() += c; return *this; }
  BufferLogger& operator<<(char c) { getString() += c; return *this; }
  BufferLogger& operator<<(BufferLogger& (*pf)(BufferLogger&)) {return pf(*this);}
  void flush();

private:
  void doit();
  void trueFlush();
  std::string& getString() { return m_whichStr ? m_strA : m_strB; };
  std::string& getFlushString() { return m_whichStr ? m_strB : m_strA; };

  bool m_bDone = false;
  std::ofstream m_ofstream;
  std::thread m_worker;
  std::condition_variable m_shouldWrite;
  static constexpr std::size_t MAX_SIZE = 3 * 1024;

  bool m_whichStr = true;
  std::string m_strA;
  std::string m_strB;
};

} // matan

namespace std {

inline matan::BunchLogger& endl(matan::BunchLogger& logger) {
  logger << '\n';
  logger.flush();
  return logger;
}

inline matan::BufferLogger& endl(matan::BufferLogger& logger) {
  logger << '\n';
  logger.flush();
  return logger;
}

}
