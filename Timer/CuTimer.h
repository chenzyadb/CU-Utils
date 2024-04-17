// CuTimer by chenzyadb@github.com
// Based on C++17 STL (MSVC)

#ifndef _CU_TIMER_
#define _CU_TIMER_

#include <chrono>
#include <thread>
#include <mutex>
#include <functional>

namespace CU
{
	class TimerExcept : public std::exception
	{
		public:
			TimerExcept(const std::string &message) : message_(message) { }

			const char* what() const noexcept override
			{
				return message_.c_str();
			}

		private:
			const std::string message_;
	};

	class Timer
	{
		public:
			typedef std::function<void(void)> TimeOutCallback;

			static void SingleShot(time_t interval, const TimeOutCallback &callback)
			{
				std::thread timerThread([interval, callback]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(interval));
					callback();
				});
				timerThread.detach();
			}

			Timer() : interval_(0), callback_(), started_(false), mtx_(), cv_(), paused_(false) { }

			Timer(time_t interval, const TimeOutCallback &callback) : 
				interval_(interval), 
				callback_(callback),
				started_(false),
				mtx_(),
				cv_(),
				paused_(false)
			{ }

			~Timer()
			{
				if (started_) {
					stop();
				}
			}

			void setInterval(time_t interval)
			{
				interval_ = interval;
			}

			void setTimeOutCallback(const TimeOutCallback &callback)
			{
				if (started_) {
					throw TimerExcept("Timer already started.");
				}
				callback_ = callback;
			}

			void start()
			{
				if (started_) {
					throw TimerExcept("Timer already started.");
				}
				started_ = true;
				std::thread timerThread(std::bind(&Timer::TimerLoop_, this));
				timerThread.detach();
			}

			void pauseTimer()
			{
				if (!paused_) {
					paused_ = true;
				}
			}

			void continueTimer()
			{
				if (paused_) {
					std::unique_lock<std::mutex> lock{};
					paused_ = false;
					cv_.notify_all();
				}
			}

			void stop()
			{
				if (!started_) {
					throw TimerExcept("Timer has not been started.");
				}
				if (paused_) {
					continueTimer();
				}
				started_ = false;
				std::this_thread::sleep_for(std::chrono::milliseconds(interval_));
			}

		private:
			void TimerLoop_()
			{
				for (;;) {
					{
						std::unique_lock<std::mutex> lock(mtx_);
						while (paused_) {
							cv_.wait(lock);
						}
					}
					if (!started_) {
						break;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(interval_));
					callback_();
				}
			}

			volatile time_t interval_;
			TimeOutCallback callback_;
			volatile bool started_;
			std::mutex mtx_;
			std::condition_variable cv_;
			volatile bool paused_;
	};
}

#endif // _CU_TIMER_
