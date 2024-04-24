// CuTimer by chenzyadb@github.com
// Based on C++17 STL (MSVC)

#ifndef _CU_TIMER_
#define _CU_TIMER_

#include <chrono>
#include <thread>
#include <mutex>
#include <functional>

#define TIMER_LOOP(TIMER) while(((TIMER)._Get_Instance())->_Timer_Condition())

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

			class _Timer_Instance
			{
				public:
					_Timer_Instance() :
						interval_(0),
						loop_(),
						threadId_(),
						mtx_(),
						cv_(),
						paused_(false),
						destroyed_(false)
					{ }

					bool _Timer_Condition() noexcept
					{
						if (paused_) {
							std::unique_lock<std::mutex> lock(mtx_);
							while (paused_) {
								cv_.wait(lock);
							}
						}
						if (std::this_thread::get_id() != threadId_) {
							if (destroyed_) {
								delete this;
							}
							return false;
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(interval_));
						return true;
					}

					void _Set_Interval(time_t interval) noexcept
					{
						interval_ = interval;
					}

					void _Set_Loop(const Task &loop)
					{
						if (threadId_ != std::thread::id()) {
							throw TimerExcept("Timer already started.");
						}
						loop_ = loop;
					}

					void _Start()
					{
						if (threadId_ != std::thread::id()) {
							throw TimerExcept("Timer already started.");
						}
						if (!loop_) {
							throw TimerExcept("Task not exist.");
						}
						std::thread timerThread(loop_);
						threadId_ = timerThread.get_id();
						timerThread.detach();
					}

					void _Stop()
					{
						if (threadId_ == std::thread::id()) {
							throw TimerExcept("Timer already stoped.");
						}
						threadId_ = std::thread::id();
						if (paused_) {
							_Continue();
						}
					}

					void _Pause()
					{
						if (threadId_ == std::thread::id()) {
							throw TimerExcept("Timer has not been started.");
						}
						paused_ = true;
					}

					void _Continue()
					{
						if (threadId_ == std::thread::id()) {
							throw TimerExcept("Timer has not been started.");
						}
						if (paused_) {
							std::unique_lock<std::mutex> lock{};
							paused_ = false;
							cv_.notify_all();
						}
					}

					void _Destroy()
					{
						destroyed_ = true;
						if (threadId_ != std::thread::id()) {
							_Stop();
						} else {
							delete this;
						}
					}

				private:
					volatile time_t interval_;
					Task loop_;
					std::thread::id threadId_;
					std::mutex mtx_;
					std::condition_variable cv_;
					volatile bool paused_;
					volatile bool destroyed_;
			};

			static void SingleShot(time_t interval, const Task &callback)
			{
				std::thread timerThread([interval, callback]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(interval));
					callback();
				});
				timerThread.detach();
			}

			Timer() : instance_(new _Timer_Instance()) { }

			~Timer()
			{
				instance_->_Destroy();
			}

			void setInterval(time_t interval) noexcept
			{
				instance_->_Set_Interval(interval);
			}

			void setLoop(const Task &loop)
			{
				instance_->_Set_Loop(loop);
			}

			void setTimeOutCallback(const Task &callback)
			{
				instance_->_Set_Loop([this, callback]() {
					TIMER_LOOP(*this) {
						callback();
					}
				});
			}

			void start()
			{
				instance_->_Start();
			}

			void stop()
			{
				instance_->_Stop();
			}

			void pauseTimer()
			{
				instance_->_Pause();
			}

			void continueTimer()
			{
				instance_->_Continue();
			}

			_Timer_Instance* _Get_Instance() noexcept
			{
				return instance_;
			}

		private:
			_Timer_Instance* instance_;
	};
}

#endif // _CU_TIMER_
