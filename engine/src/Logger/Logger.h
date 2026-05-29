#ifndef PIPEFRAME_LOGGER_H
#define PIPEFRAME_LOGGER_H

#include <string>
#include <vector>

enum LogType
{
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

struct LogEntry
{
    LogType type;
    std::string message;
};

class Logger
{
public:
    static std::vector<LogEntry> messages;

    static void Log(const std::string& message);
    static void Warn(const std::string& message);
    static void Err(const std::string& message);

private:
    static void Push(LogType type, const std::string& label, const std::string& color, const std::string& message);
};

#endif // PIPEFRAME_LOGGER_H
