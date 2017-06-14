#include "Logger.hh"

namespace matan {


/**********************    BunchLogger    *******************************/


Logger::Logger(const std::string& ofname) :
    m_ofstream(ofname, std::ios::out),
    m_worker([this]() { this->doit();}) {}

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
  m_logs.push_back(m_buf);
  m_mtx.lock();
  m_shouldWrite.notify_one();
  m_mtx.unlock();
  m_buf.clear();
}

void Logger::doit() {
  std::unique_lock<std::mutex> lock(m_mtx);
  lock.unlock();
  while (true) {
    /*
     * Need to make sure that we don't have a situation where we return empty,
     * but before the logging thread waits the writer thread pushes back a new
     * log and calls notify_one since then this log would be waiting when it
     * should have been notified.
     */
    lock.lock();
    if (m_logs.empty()) {
      m_shouldWrite.wait(lock);
    }
    lock.unlock();
    trueFlush();
    if (m_bDone) {
      break;
    }
    trueFlush();
  }
}

void Logger::trueFlush() {
  /*
   * part of worker thread that does the actual flushing to file.
   */
  for (const auto& line : m_logs.takeQueue()) {
    m_ofstream << line;
    m_ofstream.flush();
  }
}

} // matan
