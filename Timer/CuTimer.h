// CuTimer by chenzyadb@github.com
// Based on C++11 STL (GNUC)

#if !defined(__CU_TIMER__)
#define __CU_TIMER__ 1

#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <algorithm>

namespace CU 
{
    inline time_t _Get_Time()
    {
        auto time_pt = std::chrono::system_clock::now();
        auto time_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(time_pt);
        return time_ms.time_since_epoch().count();
    }

    class _Task_Scheduler 
    {
        public:
            static void AddTask(const std::function<void(void)> &task, time_t targetTime)
            {
                Instance_().addTask_(task, targetTime);
            }

        private:
            _Task_Scheduler() : taskQueue_(), cond_(), queueMutex_(), condMutex_()
            {
                std::thread(std::bind(&_Task_Scheduler::mainLoop_, this)).detach();
            }

            _Task_Scheduler(_Task_Scheduler &) = delete;
            _Task_Scheduler &operator=(_Task_Scheduler &) = delete;

            static _Task_Scheduler &Instance_()
            {
                static _Task_Scheduler instance{};
                return instance;
            }

            void addTask_(const std::function<void(void)> &task, time_t targetTime)
            {
                std::vector<std::pair<time_t, std::function<void(void)>>> taskQueue{};
                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    taskQueue = taskQueue_;
                }
                auto taskPair = std::make_pair(targetTime, task);
                if (taskQueue.size() == 0 || targetTime <= taskQueue.front().first) {
                    taskQueue.insert(taskQueue.begin(), taskPair);
                } else {
                    for (auto iter = (taskQueue.end() - 1); iter >= taskQueue.begin(); --iter) {
                        if (iter->first <= targetTime) {
                            taskQueue.insert((iter + 1), taskPair);
                            break;
                        }
                    }
                }
                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    taskQueue_ = taskQueue;
                }
                if (taskQueue.size() == 1 || targetTime <= (taskQueue.begin() + 1)->first || targetTime <= _Get_Time()) {
                    std::unique_lock<std::mutex> lock(condMutex_);
                    cond_.notify_all();
                }
            }

            void mainLoop_()
            {
                for (;;) {
                    std::function<void(void)> task{};
                    time_t nextContTime = -1;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        if (taskQueue_.size() > 0) {
                            auto iter = taskQueue_.begin();
                            if (iter->first <= _Get_Time()) {
                                task = iter->second;
                                taskQueue_.erase(iter);
                            }
                        }
                        if (taskQueue_.size() > 0) {
                            nextContTime = taskQueue_.front().first;
                        }
                    }
                    if (task != nullptr) {
                        task();
                    }
                    {
                        std::unique_lock<std::mutex> lock(condMutex_);
                        if (nextContTime != -1) {
                            auto interval = nextContTime - _Get_Time();
                            if (interval > 0) {
                                cond_.wait_for(lock, std::chrono::milliseconds(interval));
                            }
                        } else {
                            cond_.wait(lock);
                        }
                    }
                }
            }

            std::vector<std::pair<time_t, std::function<void(void)>>> taskQueue_;
            std::condition_variable cond_;
            std::mutex queueMutex_;
            std::mutex condMutex_;
    };

    class Timer
    {
        public:
            typedef std::function<void(void)> Task;
            typedef std::chrono::milliseconds Duration;

            static void SingleShot(const Task &task, time_t delayTime)
            {
                _Task_Scheduler::AddTask(task, (_Get_Time() + delayTime));
            }

            Timer() : 
                condMutex_(),
                taskMutex_(),
                cond_(),
                task_(),
                started_(false),
                paused_(false),
                stopRequested_(false),
                interval_(0),
                lastContTime_(0)
            { }

            Timer(const Task &task, time_t interval) :
                condMutex_(),
                taskMutex_(),
                cond_(),
                task_(task),
                started_(false),
                paused_(false),
                stopRequested_(false),
                interval_(interval),
                lastContTime_(0)
            { }

            ~Timer() 
            {
                stop();
            }

            void setTask(const Task &task)
            {
                std::unique_lock<std::mutex> lock(taskMutex_);
                task_ = task;
            }

            void setInterval(time_t interval)
            {
                interval_ = interval;
                if (started_ && !paused_) {
                    std::unique_lock<std::mutex> lock(condMutex_);
                    cond_.notify_all();
                }
            }

            void start()
            {
                if (!started_) {
                    std::thread(std::bind(&CU::Timer::loop_, this)).detach();
                }
            }

            void runLoop(const Task &loop) 
            {
                if (!started_) {
                    std::thread([this, loop]() {
                        started_ = true;
                        if (loop != nullptr) {
                            loop();
                        }
                        started_ = false;
                    }).detach();
                }
            }

            void stop()
            {
                if (started_) {
                    cont();
                    stopRequested_ = true;
                    while (started_) {
                        std::this_thread::yield();
                    }
                    stopRequested_ = false;
                }
            }

            void pause()
            {
                paused_ = true;
            }

            void cont()
            {
                paused_ = false;
                lastContTime_ = 0;
                if (started_) {
                    std::unique_lock<std::mutex> lock(condMutex_);
                    cond_.notify_all();
                }
            }

            bool loopCondition() 
            {
                {
                    std::unique_lock<std::mutex> lock(condMutex_);
                    if (paused_) {
                        while (paused_) {
                            cond_.wait(lock);
                        }
                    } else {
                        auto waitDuration = lastContTime_ + interval_ - _Get_Time();
                        while (waitDuration > 0) {
                            cond_.wait_for(lock, Duration(waitDuration));
                            waitDuration = lastContTime_ + interval_ - _Get_Time();
                        }
                    }
                }
                lastContTime_ = _Get_Time();
                return !stopRequested_;
            }

        private:
            mutable std::mutex condMutex_;
            mutable std::mutex taskMutex_;
            std::condition_variable cond_;
            Task task_;
            volatile bool started_;
            volatile bool paused_;
            volatile bool stopRequested_;
            volatile time_t interval_;
            volatile time_t lastContTime_;

            void loop_()
            {
                started_ = true;
                while (loopCondition()) {
                    std::unique_lock<std::mutex> lock(taskMutex_);
                    if (task_ != nullptr) {
                        task_();
                    }
                }
                started_ = false;
            }
    };
}

#define CU_TIMER_LOOP(timer) while((timer).loopCondition())

#endif // !defined(__CU_TIMER__)
