#include "Logger.h"

#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <format>
#include <syncstream>
#ifdef _WIN32
#include <windows.h>
#endif

void Logger::Init()
{
    Get();

#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    SetConsoleMode(hOut, dwMode);
#endif
}

struct Logger::Impl
{
    std::queue<LogMessage> queue;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::jthread worker;
    bool isRunning = true;

    void processQueue()
    {
        while (true)
        {
            std::unique_lock lock(queueMutex);
            cv.wait(lock, [this] { return !queue.empty() || !isRunning; });

            if (queue.empty() && !isRunning) break;

            while (!queue.empty())
            {
                auto [level, text, file, line] = std::move(queue.front());
                queue.pop();
                lock.unlock();

                std::string_view color;
                switch (level)
                {
                case LogLevel::Info: color = "\x1B[32m";
                    break; // Green
                case LogLevel::Warning: color = "\x1B[33m";
                    break; // Yellow
                case LogLevel::Error: color = "\x1B[91m";
                    break; // Red
                }

                // Formatting Logic
                std::string_view lvlStr = (level == LogLevel::Info)
                                              ? "INF"
                                              : (level == LogLevel::Warning ? "WRN" : "ERR");

                std::osyncstream(std::cout) << std::format(
                    "{}[{}] [{}:{}] {}\033[0m\n",
                    color, lvlStr, file, line, text
                );

                lock.lock();
            }
        }
    }
};

Logger::Logger() : m_impl(std::make_unique<Impl>())
{
    m_impl->worker = std::jthread([this] { m_impl->processQueue(); });
}

Logger::~Logger()
{
    {
        std::lock_guard lock(m_impl->queueMutex);
        m_impl->isRunning = false;
    }
    m_impl->cv.notify_one();
}

Logger& Logger::Get()
{
    static Logger instance;
    return instance;
}

void Logger::Log(LogLevel level, const std::string_view message, const std::source_location& loc)
{
    const auto& impl = Get().m_impl;
    {
        std::lock_guard lock(impl->queueMutex);
        impl->queue.emplace(
            level,
            std::string(message),
            loc.file_name(), static_cast<int>(loc.line())
        );
    }
    impl->cv.notify_one();
}
