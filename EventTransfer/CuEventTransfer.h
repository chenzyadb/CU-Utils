// CuEventTransfer by chenzyadb.
// Based on C++17 STL (GNUC).

#if !defined(__CU_EVENT_TRANSFER__)
#define __CU_EVENT_TRANSFER__ 1

#include <shared_mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>

namespace CU
{
    class EventTransfer
    {
        public:
            typedef size_t Handle;
            typedef const void* TransData;
            typedef std::function<void(TransData)> Subscriber;

            static Handle Subscribe(const std::string &event, const Subscriber &subscriber)
            {
                return Instance_().addSubscriber_(event, subscriber);
            }

            static void Unsubscribe(const std::string &event, Handle Handle)
            {
                Instance_().removeSubscriber_(event, Handle);
            }

            template <typename _Data_Ty>
            static void Post(const std::string &event, const _Data_Ty &data)
            {
                auto transData = reinterpret_cast<TransData>(std::addressof(data));
                Instance_().postEvent_(event, transData);
            }

            template <typename _Data_Ty>
            static const _Data_Ty &GetData(TransData transData) noexcept
            {
                return *reinterpret_cast<const _Data_Ty*>(transData);
            }

        private:
            EventTransfer() : subscribers_(), mutex_() { }
            EventTransfer(EventTransfer &other) = delete;
            EventTransfer &operator=(EventTransfer &other) = delete;

            static EventTransfer &Instance_()
            {
                static EventTransfer instance{};
                return instance;
            }

            Handle addSubscriber_(const std::string &event, const Subscriber &subscriber)
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                auto &subscribers = subscribers_[event];
                subscribers.emplace_back(subscriber);
                return (subscribers.size() - 1);
            }

            void removeSubscriber_(const std::string &event, Handle Handle)
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                auto &subscribers = subscribers_[event];
                if (subscribers.size() > Handle) {
                    subscribers.erase(subscribers.begin() + Handle);
                }
            }

            void postEvent_(const std::string &event, TransData transData) const
            {
                std::vector<Subscriber> subscribers{};
                {
                    std::shared_lock<std::shared_mutex> lock(mutex_);
                    auto iter = subscribers_.find(event);
                    if (iter != subscribers_.end()) {
                        subscribers = iter->second;
                    }
                }
                for (const auto &subscriber : subscribers) {
                    subscriber(transData);
                }
            }

            std::unordered_map<std::string, std::vector<Subscriber>> subscribers_;
            mutable std::shared_mutex mutex_;
    };
}

#endif // !defined(__CU_EVENT_TRANSFER__)
