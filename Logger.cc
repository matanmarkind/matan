#include "Logger.hh"

namespace matan {


/**********************    BunchLogger    *******************************/


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

void BunchLogger::flush() {
  m_logs.push_back(m_buf);
  m_shouldWrite.notify_one();
  m_buf.clear();
}

void BunchLogger::doit() {
  std::mutex mut;
  std::unique_lock<std::mutex> lock(mut);
  while (true) {
    if (m_bWait || m_logs.empty()) {
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


/**********************    BufferLogger    *******************************/


BufferLogger::BufferLogger(const std::string& ofname) :
    m_ofstream(ofname, std::ios::out),
    m_worker([this]() { this->doit();}) {}

BufferLogger::~BufferLogger() {
  m_bDone = true;
  trueFlush();
  m_worker.join(); //Perhaps I should detach?
  m_ofstream.close();
}

void BufferLogger::flush() {
  auto& str = getString();
  if (str.size() > MAX_SIZE) {
    trueFlush();
  }
}

void BufferLogger::trueFlush() {
  m_whichStr = !m_whichStr;
  m_shouldWrite.notify_one();
  getString().clear();
}

void BufferLogger::doit() {
  std::mutex mut;
  std::unique_lock<std::mutex> lock(mut);
  while (true) {
    m_shouldWrite.wait(lock);
    m_ofstream << getFlushString();
    m_ofstream.flush();
    if (m_bDone) {
      break;
    }
  }
}

} // matan
