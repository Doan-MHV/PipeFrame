#include "Logger.h"

#include <chrono>
#include <ctime>
#include <iostream>

namespace
{
    constexpr std::size_t MAX_LOG_MESSAGES = 1000;

    constexpr const char* COLOR_INFO = "\033[36m";
    constexpr const char* COLOR_WARN = "\033[33m";
    constexpr const char* COLOR_ERROR = "\033[31m";
    constexpr const char* COLOR_RESET = "\033[0m";

    std::string CurrentDateTimeToString()
    {
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        char buffer[32] = {};
        std::strftime(buffer, sizeof(buffer), "%d-%b-%Y %H:%M:%S", std::localtime(&now));

        return buffer;
    }
}

std::vector<LogEntry> Logger::messages;

void Logger::Push(
    LogType type,
    const std::string& label,
    const std::string& color,
    const std::string& message
)
{
    LogEntry logEntry;
    logEntry.type = type;
    logEntry.message = "[" + label + "] [" + CurrentDateTimeToString() + "] " + message;

    if (messages.size() >= MAX_LOG_MESSAGES)
    {
        messages.erase(messages.begin());
    }

    messages.push_back(logEntry);

    std::ostream& output = (type == LOG_ERROR) ? std::cerr : std::cout;
    output << color << logEntry.message << COLOR_RESET << std::endl;
}

void Logger::Log(const std::string& message)
{
    Push(LOG_INFO, "INFO", COLOR_INFO, message);
}

void Logger::Warn(const std::string& message)
{
    Push(LOG_WARNING, "WARN", COLOR_WARN, message);
}

void Logger::Err(const std::string& message)
{
    Push(LOG_ERROR, "ERROR", COLOR_ERROR, message);
}
