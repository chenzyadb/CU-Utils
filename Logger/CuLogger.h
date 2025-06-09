// CuLogger by chenzyadb.
// Based on C++17 STL (GNUC) & CuFormat

#if !defined(_CU_LOGGER_)
#define _CU_LOGGER_ 1

#if defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif // defined(_MSC_VER)

#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <memory>
#include "CuFormat.h"

namespace CU
{
    class Logger
    {
        public:
            enum class LogLevel : uint8_t {NONE, ERROR, WARN, INFO, DEBUG, VERBOSE};

            static void Create(LogLevel level, const std::string &path)
            {
                Instance_().setLogger_(level, path);
            }

            template <typename ..._Args>
            static void Error(std::string_view format, const _Args &...args)
            {
                Instance_().joinLogQueue_(LogLevel::ERROR, CU::Format(format, args...));
            }

            template <typename ..._Args>
            static void Warn(std::string_view format, const _Args &...args)
            {
                Instance_().joinLogQueue_(LogLevel::WARN, CU::Format(format, args...));
            }

            template <typename ..._Args>
            static void Info(std::string_view format, const _Args &...args)
            {
                Instance_().joinLogQueue_(LogLevel::INFO, CU::Format(format, args...));
            }

            template <typename ..._Args>
            static void Debug(std::string_view format, const _Args &...args)
            {
                Instance_().joinLogQueue_(LogLevel::DEBUG, CU::Format(format, args...));
            }

            template <typename ..._Args>
            static void Verbose(std::string_view format, const _Args &...args)
            {
                Instance_().joinLogQueue_(LogLevel::VERBOSE, CU::Format(format, args...));
            }

            static void Flush()
            {
                Instance_().flushLogQueue_();
            }

        private:
            Logger() : 
                logPath_(), 
                logLevel_(), 
                logQueue_(), 
                queueMutex_(),
                flushMutex_(),
                queueCond_(),
                flushCond_()
            { }
            
            Logger(Logger &other) = delete;
            Logger &operator=(Logger &other) = delete;

            static Logger &Instance_()
            {
                static Logger instance{};
                return instance;
            }

            void setLogger_(LogLevel level, const std::string &path)
            {
                static const auto createFile = [](const std::string &filePath) -> bool {
                    auto fp = std::fopen(filePath.data(), "wt");
                    if (fp != nullptr) {
                        std::fclose(fp);
                        return true;
                    }
                    return false;
                };

                if (logLevel_ == LogLevel::NONE && level != LogLevel::NONE) {
                    logLevel_ = level;
                    logPath_ = path;
                    if (createFile(logPath_)) {
                        std::thread(std::bind(&Logger::mainLoop_, this)).detach();
                    }
                }
            }

            void mainLoop_()
            {
                static const auto flushed = [this]() {
                    std::unique_lock lock(flushMutex_);
                    flushCond_.notify_all();
                };

                auto fp = std::fopen(logPath_.data(), "at");
                if (fp == nullptr) {
                    return;
                }
                std::vector<std::string> writeQueue{};
                for (;;) {
                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        while (logQueue_.empty()) {
                            flushed();
                            queueCond_.wait(lock);
                        }
                        writeQueue.swap(logQueue_);
                    }
                    if (!writeQueue.empty()) {
                        for (const auto &logText : writeQueue) {
                            std::fputs(logText.data(), fp);
                        }
                        std::fflush(fp);
                        writeQueue.clear();
                    }
                }
            }

            void joinLogQueue_(LogLevel level, const std::string &content)
            {
                static const std::unordered_map<LogLevel, const char*> levelStrings = {
                    {LogLevel::ERROR, " [E] "},
                    {LogLevel::WARN, " [W] "},
                    {LogLevel::INFO, " [I] "},
                    {LogLevel::DEBUG, " [D] "},
                    {LogLevel::VERBOSE, " [V] "}
                };

                static const auto getTimeInfo = []() -> std::string {
                    auto time_ptr = std::chrono::system_clock::now();
                    auto time = std::chrono::system_clock::to_time_t(time_ptr);
                    auto localTime = std::localtime(std::addressof(time));
                    return CU::CFormat("%02d-%02d %02d:%02d:%02d",
                        (localTime->tm_mon + 1), localTime->tm_mday, localTime->tm_hour, 
                        localTime->tm_min, localTime->tm_sec);
                };

                if (level <= logLevel_) {
                    std::string logText{};
                    logText.reserve(content.size() + sizeof("00-00 00:00:00 [_] \n"));
                    logText.append(getTimeInfo());
                    logText.append(levelStrings.at(level));
                    logText.append(content);
                    logText.append("\n");

                    std::unique_lock<std::mutex> lock(queueMutex_);
                    logQueue_.emplace_back(std::move(logText));
                    queueCond_.notify_all();
                }
            }

            void flushLogQueue_()
            {
                std::unique_lock lock(flushMutex_);
                flushCond_.wait(lock);
            }

            std::string logPath_;
            LogLevel logLevel_;
            std::vector<std::string> logQueue_;
            std::mutex queueMutex_;
            std::mutex flushMutex_;
            std::condition_variable queueCond_;
            std::condition_variable flushCond_;
        };
}

#endif // !defined(_CU_LOGGER_)
