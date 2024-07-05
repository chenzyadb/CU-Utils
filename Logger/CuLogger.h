// CuLogger V3 by chenzyadb.
// Based on C++11 STL (GNUC).

#if !defined(_CU_LOGGER_)
#define _CU_LOGGER_ 1

#if defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif // defined(_MSC_VER)

#include <chrono>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <thread>
#include <functional>
#include <exception>
#include <memory>
#include <cstdio>
#include <cstdarg>
#include <cstring>

namespace CU
{
	class Logger
	{
		public:
			enum class LogLevel : uint8_t {NONE, ERROR, WARN, INFO, DEBUG, VERBOSE};

			static void Create(const LogLevel &level, const std::string &path)
			{
				Instance_().setLogger_(level, path);
			}

			static void Error(const char* format, ...)
			{
				va_list args{};
				va_start(args, format);
				int len = std::vsnprintf(nullptr, 0, format, args) + 1;
				va_end(args);
				if (len > 1) {
					auto buffer = new char[len];
					std::memset(buffer, 0, len);
					va_start(args, format);
					std::vsnprintf(buffer, len, format, args);
					va_end(args);
					Instance_().joinLogQueue_(LogLevel::ERROR, buffer);
					delete[] buffer;
				}
			}

			static void Warn(const char* format, ...)
			{
				va_list args{};
				va_start(args, format);
				int len = std::vsnprintf(nullptr, 0, format, args);
				va_end(args);
				if (len > 0) {
					int size = len + 1;
					auto buffer = new char[size];
					std::memset(buffer, 0, size);
					va_start(args, format);
					std::vsnprintf(buffer, size, format, args);
					va_end(args);
					Instance_().joinLogQueue_(LogLevel::WARN, buffer);
					delete[] buffer;
				}
			}

			static void Info(const char* format, ...)
			{
				va_list args{};
				va_start(args, format);
				int len = std::vsnprintf(nullptr, 0, format, args);
				va_end(args);
				if (len > 0) {
					int size = len + 1;
					auto buffer = new char[size];
					std::memset(buffer, 0, size);
					va_start(args, format);
					std::vsnprintf(buffer, size, format, args);
					va_end(args);
					Instance_().joinLogQueue_(LogLevel::INFO, buffer);
					delete[] buffer;
				}
			}

			static void Debug(const char* format, ...)
			{
				va_list args{};
				va_start(args, format);
				int len = std::vsnprintf(nullptr, 0, format, args);
				va_end(args);
				if (len > 0) {
					int size = len + 1;
					auto buffer = new char[size];
					std::memset(buffer, 0, size);
					va_start(args, format);
					std::vsnprintf(buffer, size, format, args);
					va_end(args);
					Instance_().joinLogQueue_(LogLevel::DEBUG, buffer);
					delete[] buffer;
				}
			}

			static void Verbose(const char* format, ...)
			{
				va_list args{};
				va_start(args, format);
				int len = std::vsnprintf(nullptr, 0, format, args);
				va_end(args);
				if (len > 0) {
					int size = len + 1;
					auto buffer = new char[size];
					std::memset(buffer, 0, size);
					va_start(args, format);
					std::vsnprintf(buffer, size, format, args);
					va_end(args);
					Instance_().joinLogQueue_(LogLevel::VERBOSE, buffer);
					delete[] buffer;
				}
			}

			static void Flush()
			{
				Instance_().flushLogQueue_();
			}

		private:
			Logger() : logPath_(), logLevel_(), cv_(), mtx_(), logQueue_(), queueFlushed_(true) { }
			Logger(Logger &) = delete;
			Logger &operator=(Logger &) = delete;

			static Logger &Instance_()
			{
				static Logger instance{};
				return instance;
			}

			void setLogger_(const LogLevel &level, const std::string &path)
			{
				static const auto createFile = [](const std::string &filePath) -> bool {
					auto fp = std::fopen(filePath.c_str(), "wt");
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
						std::thread mainLoop(std::bind(&Logger::mainLoop_, this));
						mainLoop.detach();
					}
				}
			}

			void mainLoop_()
			{
				auto fp = std::fopen(logPath_.c_str(), "at");
				if (fp == nullptr) {
					return;
				}
				std::vector<std::string> writeQueue{};
				for (;;) {
					{
						std::unique_lock<std::mutex> lck(mtx_);
						queueFlushed_ = logQueue_.empty();
						while (logQueue_.empty()) {
							cv_.wait(lck);
						}
						writeQueue = logQueue_;
						logQueue_.clear();
					}
					if (!writeQueue.empty()) {
						for (const auto &log : writeQueue) {
							std::fputs(log.c_str(), fp);
						}
						std::fflush(fp);
						writeQueue.clear();
					}
				}
			}

			void joinLogQueue_(const LogLevel &level, const char* content)
			{
				static const auto getTimeInfo = []() -> std::string {
					auto now = std::chrono::system_clock::now();
					auto nowTime = std::chrono::system_clock::to_time_t(now);
					auto localTime = std::localtime(std::addressof(nowTime));
					char buffer[16] = { 0 };
					std::snprintf(buffer, sizeof(buffer), "%02d-%02d %02d:%02d:%02d",
						localTime->tm_mon + 1, localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
					return buffer;
				};
				{
					std::unique_lock<std::mutex> lck(mtx_);
					if (level <= logLevel_) {
						switch (level) {
							case LogLevel::ERROR:
								logQueue_.emplace_back(getTimeInfo() + " [E] " + content + '\n');
								break;
							case LogLevel::WARN:
								logQueue_.emplace_back(getTimeInfo() + " [W] " + content + '\n');
								break;
							case LogLevel::INFO:
								logQueue_.emplace_back(getTimeInfo() + " [I] " + content + '\n');
								break;
							case LogLevel::DEBUG:
								logQueue_.emplace_back(getTimeInfo() + " [D] " + content + '\n');
								break;
							case LogLevel::VERBOSE:
								logQueue_.emplace_back(getTimeInfo() + " [V] " + content + '\n');
								break;
							default:
								break;
						}
						cv_.notify_all();
					}
				}
			}

			void flushLogQueue_()
			{
				bool flushed = false;
				while (!flushed) {
					{
						std::unique_lock<std::mutex> lck(mtx_);
						flushed = queueFlushed_;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
			}

			std::string logPath_;
			LogLevel logLevel_;
			std::condition_variable cv_;
			std::mutex mtx_;
			std::vector<std::string> logQueue_;
			volatile bool queueFlushed_;
		};
}

#endif // !defined(_CU_LOGGER_)
