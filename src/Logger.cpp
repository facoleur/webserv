#include "Logger.hpp"
#include "Utils.hpp"
#include <string>

void Logger::log(const LogLevel level, const std::string& message) {
    if (level > LOG_ERROR)
        return;

    std::string timestamp = getCurrentDatetime("%F %T ");
    std::string lvlstr;

    switch (level) {
        case LOG_INFO:
            lvlstr = "INFO";
            break;
        case LOG_WARNING:
            lvlstr = "WARNING";
            break;
        case LOG_DEBUG:
            lvlstr = "DEBUG";
            break;
        default:
            lvlstr = "ERROR";
    }

    std::cout << timestamp << "[" << lvlstr << "] " << message << std::endl;
}
