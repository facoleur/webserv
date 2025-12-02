#pragma once

#include <string>

enum LogLevel { LOG_INFO, LOG_WARNING, LOG_ERROR };

class Logger {
  public:
    static void log(const LogLevel level, const std::string& message);

  private:
    static void writeline(const std::string& line);
};

#define LOG_INFO(message) Logger::log(LOG_INFO, message);
#define LOG_WARNING(message) Logger::log(LOG_WARNING, message);
#define LOG_ERROR(message) Logger::log(LOG_ERROR, message);
