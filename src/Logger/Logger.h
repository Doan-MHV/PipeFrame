#ifndef PIPEFRAME_LOGGER_H
#define PIPEFRAME_LOGGER_H

#include <string>
#include <string_view>
#include <source_location>
#include <memory>
#include <format>

enum class LogLevel
{
    Info, Warning, Error
};

struct LogMessage
{
    LogLevel level;
    std::string text;
    std::string file;
    int line;
};

class Logger
{
public:
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;

    static void Init();
    static void Shutdown();

    static void Log(LogLevel level, std::string_view message,
                    const std::source_location& loc = std::source_location::current());

private:
    Logger();
    ~Logger();

    struct Impl;
    std::unique_ptr<Impl> m_impl;

    static Logger& Get();
};

#define PF_INFO(msg)  Logger::Log(LogLevel::Info, msg)
#define PF_WARN(msg)  Logger::Log(LogLevel::Warning, msg)
#define PF_ERROR(msg) Logger::Log(LogLevel::Error, msg)


#endif //PIPEFRAME_LOGGER_H
