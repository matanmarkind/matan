#include "Logger.hh"


namespace matan {


/**********************    BunchLogger    *******************************/


Logger::Logger(const std::string& ofname) :
    AsyncWorker(),
    m_ofstream(ofname, std::ios::out) {}

Logger::~Logger() {
  std::cout << "~Logger" << std::endl;
  m_bDone = true;
  std::cout << "m_bDone" << std::endl;
  doFlush();
  std::cout << "pre-join" << std::endl;
  if (m_worker.joinable()) {
    std::cout << "try-join" << std::endl;
    m_worker.join(); //Perhaps I should detach?
    std::cout << "did-join" << std::endl;
  }
  std::cout << "post-join" << std::endl;
  m_ofstream.close();
}

void Logger::flush() {
  if (m_buf.size() > MAX_LEN) {
    doFlush();
  }
}

void Logger::doFlush() {
  std::cout << "doFlush" << std::endl;
  m_contents.push_back(m_buf);
  notifyNewEle();
  m_buf.clear();
}

void Logger::doit() {
  std::cout << "doit" << std::endl;
  for (const auto& line : m_contents.takeQueue()) {
    m_ofstream << line;
    m_ofstream.flush();
  }
}

} // matan
