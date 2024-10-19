#ifndef SIMPLELOGGER_H
#define SIMPLELOGGER_H

#include <ctime>
#include <fstream>
#include <locale>
#include "colorer/common/Logger.h"

class SimpleLogger : public Logger
{
 public:
  static constexpr std::string_view LogLevelStr[] {"off", "error", "warning", "info", "debug", "trace"};

  SimpleLogger(const std::string& filename, const std::string& log_level)
  {
    current_level = getLogLevel(log_level);
    ofs.open(filename.c_str(), std::ofstream::out | std::ofstream::app);
  }

  SimpleLogger(const std::string& filename, const Logger::LogLevel log_level)
  {
    current_level = log_level;
    ofs.open(filename.c_str(), std::ofstream::out | std::ofstream::app);
  }

  ~SimpleLogger() override
  {
    ofs.close();
  }

  void log(Logger::LogLevel level, const char* /*filename_in*/, int /*line_in*/, const char* /*funcname_in*/, const char* message)
  {
    if (level > current_level) {
      return;
    }
    std::time_t const t = std::time(nullptr);
    char mbstr[30];
    std::strftime(mbstr, sizeof(mbstr), "%FT%T", std::localtime(&t));
    ofs << "[" << mbstr << "] "
        << "[" << LogLevelStr[level] << "] ";
    ofs << message << '\n';
  }

  static Logger::LogLevel getLogLevel(const std::string& log_level)
  {
    int i = 0;
    for (auto it : LogLevelStr) {
      if (log_level == it) {
        return static_cast<Logger::LogLevel>(i);
      }
      i++;
    }
    if (log_level == "warn") {
      return Logger::LOG_WARN;
    }
    return Logger::LOG_OFF;
  }

  void setLogLevel(Logger::LogLevel level)
  {
    ofs.flush();
    current_level = level;
  }

  void flush()
  {
    ofs.flush();
  }

 private:
  std::ofstream ofs;
  Logger::LogLevel current_level;
};

#endif  // SIMPLELOGGER_H
