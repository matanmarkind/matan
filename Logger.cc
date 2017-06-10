#include "Logger.hh"

namespace matan {

BunchLogger::BunchLogger(const std::string& ofname, bool wait) :
    m_bWait(wait),
    m_ofstream(ofname, std::ios::out),
    m_worker([this]() { this->doit();}) {}

BunchLogger::~BunchLogger() {
    awake();
    m_bDone = true;
    trueFlush();
    m_worker.join(); //Perhaps I should detach?
    m_ofstream.close();
  }

void BunchLogger::doit() {
  std::mutex mut;
  std::unique_lock<std::mutex> lock(mut);
  while (true) {
    if (m_bWait) {
      m_shouldWrite.wait(lock);
    }
    trueFlush();
    if (m_bDone) {
      break;
    }
  }
}

void BunchLogger::trueFlush() {
  for (const auto& line : m_logs.takeQueue()) {
    m_ofstream << line;
    m_ofstream.flush();
  }
}

} // matan
