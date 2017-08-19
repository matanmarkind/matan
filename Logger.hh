#pragma once

#include "AsyncWorker.hh"
#include <fstream>
#include <string>

namespace matan {

class Logger final : public AsyncWorker {
  /*
   * Logger intended to prevent IO from blocking. Pushes the actual writing
   * to disk onto a separate thread.
   *
   * Single writer. To make the interface similar to std::cout we need to allow
   * separate calls to operator<<. For this to be multi writer we would need
   * each operator<< call to contain a complete elements, as opposed to
   * building it within m_buf and only later flushing it. (Please note this
   * issue would exist even if we actually flushed on every call to flush()).
   */
public:
  Logger(const std::string& ofname);
  virtual ~Logger();
  Logger(const Logger&) = delete;
  Logger& operator<<(const std::string& str) {m_buf += str; return *this;}
  Logger& operator<<(const char* c) { m_buf += c; return *this; }
  Logger& operator<<(char c) { m_buf += c; return *this; }
  Logger& operator<<(Logger& (*pf)(Logger&)) {return pf(*this);}
  void flush();

private:
  void doFlush();
  virtual void doit() override;
  virtual bool shouldSleep() const override { return m_contents.empty(); }

  std::string m_buf;
  std::ofstream m_ofstream;
  BunchQueue<std::string> m_contents;
  /*
   * I'm making a guess here that one page in memory is 4KB and that it will
   * be fastest if I can stay on one page (I need to pick a threshold
   * somehow) and that most logs will be less than 1024 characters.
   */
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
