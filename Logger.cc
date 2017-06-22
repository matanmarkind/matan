#include "Logger.hh"

namespace matan {


/**********************    BunchLogger    *******************************/


Logger::Logger(const std::string& ofname) :
    LoggerBase(),
    m_ofstream(ofname, std::ios::out) {}

Logger::~Logger() {
  m_bDone = true;
  doFlush();
  m_worker.join(); //Perhaps I should detach?
  m_ofstream.close();
}

void Logger::flush() {
  if (m_buf.size() > MAX_LEN) {
    doFlush();
  }
}

void Logger::doFlush() {
  m_contents.push_back(m_buf);
  notifyNewEle();
  m_buf.clear();
}

void Logger::doit() {
  /*
   * part of worker thread that does the actual flushing to file.
   */
  for (const auto& line : m_contents.takeQueue()) {
    m_ofstream << line;
    m_ofstream.flush();
  }
}

} // matan
