// CuProperty by chenzyadb@github.com
// Based on C++17 STL (GNUC)

#if !defined(__CU_PROPERTY__)
#define __CU_PROPERTY__ 1

#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <variant>
#include <vector>
#include <mutex>
#include <functional>

namespace CU
{
    class PropertyWatcher
    {
        public:
            enum class Event : uint8_t {SET, GET, REMOVE};

            typedef size_t Handler;
            typedef std::function<void(void)> Notifier;

            static Handler AddWatch(const std::string &propName, Event event, const Notifier &notifier)
            {
                return Instance_().addWatch_(propName, event, notifier);
            }

            static void RemoveWatch(const std::string &propName, Event event, Handler handler)
            {
                Instance_().removeWatch_(propName, event, handler);
            }

            static void _Call_Notifier(const std::string &propName, Event event)
            {
                Instance_().callNotifier_(propName, event);
            }

        private:
            PropertyWatcher() : notifiers_(), mutex_() { }
            PropertyWatcher(PropertyWatcher &other) = delete;
            PropertyWatcher &operator=(PropertyWatcher &other) = delete;

            static PropertyWatcher &Instance_()
            {
                static PropertyWatcher instance{};
                return instance;
            }

            Handler addWatch_(const std::string &propName, Event event, const Notifier &notifier)
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                auto &notifiers = notifiers_[propName][event];
                notifiers.emplace_back(notifier);
                return (notifiers.size() - 1);
            }

            void removeWatch_(const std::string &propName, Event event, Handler handler)
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                auto &notifiers = notifiers_[propName][event];
                if (notifiers.size() > handler) {
                    notifiers.erase(notifiers.begin() + handler);
                }
            }

            void callNotifier_(const std::string &propName, Event event) const
            {
                std::vector<Notifier> notifiers{};
                {
                    std::shared_lock<std::shared_mutex> lock(mutex_);
                    auto eventsIter = notifiers_.find(propName);
                    if (eventsIter != notifiers_.end()) {
                        auto notifiersIter = (eventsIter->second).find(event);
                        if (notifiersIter != (eventsIter->second).end()) {
                            notifiers = notifiersIter->second;
                        }
                    }
                }
                for (const auto &notifier : notifiers) {
                    notifier();
                }
            }

            std::unordered_map<std::string, std::unordered_map<Event, std::vector<Notifier>>> notifiers_;
            mutable std::shared_mutex mutex_;
    };

    class Property 
    {
        public:
            class Value 
            {
                public:
                    typedef std::variant<std::string, int64_t, bool, std::vector<std::string>, std::vector<int64_t>> ValueData;

                    enum class ValueType : uint8_t {NONE, STRING, INTEGER, BOOLEAN, LIST_STRING, LIST_INTEGER};

                    Value() : data_(), type_(ValueType::NONE), mutex_() { }

                    Value(const std::string &data) : data_(data), type_(ValueType::STRING), mutex_() { }

                    Value(const char* data) : data_(std::string(data)), type_(ValueType::STRING), mutex_() { }

                    Value(int64_t data) : data_(data), type_(ValueType::INTEGER), mutex_() { }

                    Value(int data) : data_(static_cast<int64_t>(data)), type_(ValueType::INTEGER), mutex_() { }

                    Value(bool data) : data_(data), type_(ValueType::BOOLEAN), mutex_() { }

                    Value(const std::vector<std::string> &data) : data_(data), type_(ValueType::LIST_STRING), mutex_() { }

                    Value(const std::vector<int64_t> &data) : data_(data), type_(ValueType::LIST_INTEGER), mutex_() { }

                    Value(const Value &other) : data_(other.data()), type_(other.type()), mutex_() { }

                    Value(Value &&other) noexcept : data_(other.data()), type_(other.type()), mutex_() { }

                    Value &operator=(const Value &other)
                    {
                        std::unique_lock<std::shared_mutex> lock(mutex_);
                        if (std::addressof(other) != this) {
                            data_ = other.data();
                            type_ = other.type();
                        }
                        return *this;
                    }

                    Value &operator=(Value &&other) noexcept
                    {
                        std::unique_lock<std::shared_mutex> lock(mutex_);
                        if (std::addressof(other) != this) {
                            data_ = other.data();
                            type_ = other.type();
                        }
                        return *this;
                    }

                    bool operator==(const Value &other) const
                    {
                        std::shared_lock<std::shared_mutex> lock(mutex_);
                        if (std::addressof(other) == this) {
                            return true;
                        }
                        return (other.type() == type_ && other.data() == data_);
                    }

                    bool operator!=(const Value &other) const
                    {
                        std::shared_lock<std::shared_mutex> lock(mutex_);
                        if (std::addressof(other) == this) {
                            return false;
                        }
                        return (other.type() != type_ || other.data() != data_);
                    }

                    std::string toString() const
                    {
                        std::shared_lock<std::shared_mutex> lock(mutex_);
                        if (type_ != ValueType::STRING) {
                            return {};
                        }
                        return std::get<std::string>(data_);
                    }

                    int64_t toInteger() const
                    {
                        std::shared_lock<std::shared_mutex> lock(mutex_);
                        if (type_ != ValueType::INTEGER) {
                            return 0;
                        }
                        return std::get<int64_t>(data_);
                    }

                    bool toBoolean() const
                    {
                        std::shared_lock<std::shared_mutex> lock(mutex_);
                        if (type_ != ValueType::BOOLEAN) {
                            return false;
                        }
                        return std::get<bool>(data_);
                    }

                    std::vector<std::string> toListString() const
                    {
                        std::shared_lock<std::shared_mutex> lock(mutex_);
                        if (type_ != ValueType::LIST_STRING) {
                            return {};
                        }
                        return std::get<std::vector<std::string>>(data_);
                    }

                    std::vector<int64_t> toListInteger() const
                    {
                        std::shared_lock<std::shared_mutex> lock(mutex_);
                        if (type_ != ValueType::LIST_INTEGER) {
                            return {};
                        }
                        return std::get<std::vector<int64_t>>(data_);
                    }

                    ValueData data() const
                    {
                        std::shared_lock<std::shared_mutex> lock(mutex_);
                        return data_;
                    }

                    ValueType type() const
                    {
                        std::shared_lock<std::shared_mutex> lock(mutex_);
                        return type_;
                    }

                private:
                    ValueData data_;
                    ValueType type_;
                    mutable std::shared_mutex mutex_;
            };

            static bool Exists(const std::string &name)
            {
                return Instance_().isPropExists_(name);
            }

            static void Set(const std::string &name, const Value &value)
            {
                Instance_().setProp_(name, value);
                PropertyWatcher::_Call_Notifier(name, PropertyWatcher::Event::SET);
            }

            static const Value &Get(const std::string &name)
            {
                PropertyWatcher::_Call_Notifier(name, PropertyWatcher::Event::GET);
                return Instance_().getProp_(name);
            }

            static void Remove(const std::string &name)
            {
                Instance_().removeProp_(name);
                PropertyWatcher::_Call_Notifier(name, PropertyWatcher::Event::REMOVE);
            }

        private:
            Property() : properties_(), mutex_() { }
            Property(Property &other) = delete;
            Property &operator=(Property &other) = delete;

            static Property &Instance_()
            {
                static Property instance{};
                return instance;
            }

            bool isPropExists_(const std::string &name) const
            {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                return (properties_.count(name) != 0);
            }

            void addProp_(const std::string &name, const Value &value)
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                properties_.emplace(name, value);
            }

            void removeProp_(const std::string &name)
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                properties_.erase(name);
            }

            void setProp_(const std::string &name, const Value &value)
            {
                if (!isPropExists_(name)) {
                    addProp_(name, value);
                    return;
                }
                std::shared_lock<std::shared_mutex> lock(mutex_);
                properties_[name] = value;
            }

            const Value &getProp_(const std::string &name)
            {
                if (!isPropExists_(name)) {
                    addProp_(name, Value());
                }
                std::shared_lock<std::shared_mutex> lock(mutex_);
                return properties_.at(name);
            }

            std::unordered_map<std::string, Value> properties_;
            mutable std::shared_mutex mutex_;
    };
}

#endif // !defined(__CU_PROPERTY__)
