// CuTimer by chenzyadb@github.com
// Based on C++17 STL (MSVC)

#ifndef _CU_TIMER_
#define _CU_TIMER_

#include <chrono>
#include <thread>
#include <mutex>
#include <functional>

#define TIMER_LOOP(TIMER) while((TIMER)._Timer_Condition())

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
			typedef std::function<void(void)> Task;

			static void SingleShot(time_t interval, const Task &callback)
			{
				std::thread timerThread([interval, callback]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(interval));
					callback();
				});
				timerThread.detach();
			}

			Timer() : interval_(0), loop_(), started_(false), mtx_(), cv_(), paused_(false) { }

			~Timer()
			{
				if (started_) {
					stop();
				}
			}

			bool _Timer_Condition()
			{
				if (paused_) {
					std::unique_lock<std::mutex> lock(mtx_);
					while (paused_) {
						cv_.wait(lock);
					}
				}
				if (!started_) {
					return false;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(interval_));
				return true;
			}

			void setInterval(time_t interval)
			{
				interval_ = interval;
			}

			void setLoop(const Task &loop)
			{
				if (started_) {
					throw TimerExcept("Timer already started.");
				}
				loop_ = loop;
			}

			void setTimeOutCallback(const Task &callback)
			{
				setLoop([this, callback]() {
					TIMER_LOOP(*this) { 
						callback();
					}
				});
			}

			void start()
			{
				if (started_) {
					throw TimerExcept("Timer already started.");
				}
				if (!loop_) {
					throw TimerExcept("Task not exist.");
				}
				started_ = true;
				std::thread timerThread(loop_);
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
				started_ = false;
				if (paused_) {
					continueTimer();
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(interval_));
			}

		private:
			volatile time_t interval_;
			Task loop_;
			volatile bool started_;
			std::mutex mtx_;
			std::condition_variable cv_;
			volatile bool paused_;
	};
}

#endif // _CU_TIMER_
