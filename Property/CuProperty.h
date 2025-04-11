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
    class AnyValue
    {
        public:
            template<typename _Ty>
            static size_t TypeId() noexcept
            {
                static constexpr char data = 0;
                return reinterpret_cast<size_t>(std::addressof(data));
            }

            class Content_Base
            {
                public:
                    Content_Base() = default;

                    virtual ~Content_Base() = default;

                    virtual Content_Base* copy() const = 0;
            };

            template <typename _Content_Ty>
            class Content : Content_Base
            {
                public:
                    Content(Content &) = delete;

                    Content &operator=(Content &) = delete;

                    Content(const _Content_Ty &data) : data_(data) { }

                    Content(_Content_Ty &&data) : data_(data) { }

                    ~Content() { }

                    Content_Base* copy() const override
                    {
                        return reinterpret_cast<Content_Base*>(new Content(data_));
                    }

                    _Content_Ty &data()
                    {
                        return data_;
                    }

                    const _Content_Ty &data() const
                    {
                        return data_;
                    }

                private:
                    _Content_Ty data_;
            };

            AnyValue() : content_(nullptr), typeId_(0) { }

            template <typename _Content_Ty>
            AnyValue(const _Content_Ty &data) : content_(NewContent(data)), typeId_(TypeId<_Content_Ty>()) { }

            AnyValue(const AnyValue &other) : content_(nullptr), typeId_(other.typeId_) 
            {
                if (other.content_ != nullptr) {
                    content_ = (other.content_)->copy();
                }
            }

            AnyValue(AnyValue &&other) noexcept : content_(other.content_), typeId_(other.typeId_) 
            {
                other.content_ = nullptr;
                other.typeId_ = 0;
            }

            ~AnyValue() 
            {
                clear();
            }

            AnyValue &operator=(const AnyValue &other)
            {
                if (std::addressof(other) != this) {
                    clear();
                    if (other.content_ != nullptr) {
                        content_ = (other.content_)->copy();
                        typeId_ = other.typeId_;
                    }
                }
                return *this;
            }

            AnyValue &operator=(AnyValue &&other) noexcept
            {
                if (std::addressof(other) != this) {
                    clear();
                    content_ = other.content_;
                    typeId_ = other.typeId_;

                    other.content_ = nullptr;
                    other.typeId_ = 0;
                }
                return *this;
            }

            template <typename _Content_Ty>
            explicit operator _Content_Ty() const
            {
                return data<_Content_Ty>();
            }

            template <typename _Content_Ty>
            const _Content_Ty &data() const
            {
                if (content_ != nullptr && typeId_ == TypeId<_Content_Ty>()) {
                    return ToContent<_Content_Ty>(content_)->data();
                }
                throw std::bad_cast();
            }

            template <typename _Content_Ty>
            _Content_Ty &data()
            {
                if (content_ != nullptr && typeId_ == TypeId<_Content_Ty>()) {
                    return ToContent<_Content_Ty>(content_)->data();
                }
                throw std::bad_cast();
            }

            template <typename _Content_Ty>
            bool is() const noexcept
            {
                return (typeId_ == TypeId<_Content_Ty>());
            }

            void clear()
            {
                if (content_ != nullptr) {
                    delete content_;
                    content_ = nullptr;
                    typeId_ = 0;
                }
            }

        private:
            Content_Base* content_;
            size_t typeId_;

            template <typename _Content_Ty>
            static Content_Base* NewContent(const _Content_Ty &data)
            {
                return reinterpret_cast<Content_Base*>(new Content(data));
            }

            template <typename _Content_Ty>
            static Content<_Content_Ty>* ToContent(Content_Base* base) noexcept
            {
                return reinterpret_cast<Content<_Content_Ty>*>(base);
            }
    };

    class PropertyWatcher
    {
        public:
            enum class Event : uint8_t {SET, GET, REMOVE};

            typedef size_t Handle;
            typedef std::function<void(void)> Notifier;

            static Handle AddWatch(const std::string &propName, Event event, const Notifier &notifier)
            {
                return Instance_().addWatch_(propName, event, notifier);
            }

            static void RemoveWatch(const std::string &propName, Event event, Handle handle)
            {
                Instance_().removeWatch_(propName, event, handle);
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

            Handle addWatch_(const std::string &propName, Event event, const Notifier &notifier)
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                auto &notifiers = notifiers_[propName][event];
                notifiers.emplace_back(notifier);
                return (notifiers.size() - 1);
            }

            void removeWatch_(const std::string &propName, Event event, Handle handle)
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                auto &notifiers = notifiers_[propName][event];
                if (notifiers.size() > handle) {
                    notifiers.erase(notifiers.begin() + handle);
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
                    Value() : mutex_(), data_() { }

                    template <typename _Val_Ty>
                    Value(const _Val_Ty &data) : mutex_(), data_(data) { }

                    Value(const Value &other) : mutex_(), data_(other.data()) { }

                    Value(Value &&other) noexcept : mutex_(), data_(other.data()) { }

                    Value operator=(const Value &other)
                    {
                        std::unique_lock<std::shared_mutex> lock(mutex_);
                        if (std::addressof(other) != this) {
                            data_ = other.data();
                        }
                        return data_;
                    }

                    Value operator=(Value &&other) noexcept
                    {
                        std::unique_lock<std::shared_mutex> lock(mutex_);
                        if (std::addressof(other) != this) {
                            data_ = other.data();
                        }
                        return data_;
                    }

                    template <typename _Val_Ty>
                    operator _Val_Ty() const
                    {
                        std::shared_lock<std::shared_mutex> lock(mutex_);
                        return data_.data<_Val_Ty>();
                    }

                    template <typename _Val_Ty>
                    bool is() const
                    {
                        return data().is<_Val_Ty>();
                    }

                    AnyValue data() const
                    {
                        std::shared_lock<std::shared_mutex> lock(mutex_);
                        return data_;
                    }

                private:
                    mutable std::shared_mutex mutex_;
                    AnyValue data_;
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

            template <typename _Val_Ty>
            static _Val_Ty Get(const std::string &name)
            {
                PropertyWatcher::_Call_Notifier(name, PropertyWatcher::Event::GET);
                if (Instance_().isPropExists_(name)) {
                    const auto &value = Instance_().getProp_(name);
                    if (value.is<_Val_Ty>()) {
                        return static_cast<_Val_Ty>(value);
                    }
                }
                return {};
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

            const Value &getProp_(const std::string &name) const
            {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                return properties_.at(name);
            }

            std::unordered_map<std::string, Value> properties_;
            mutable std::shared_mutex mutex_;
    };
}

#endif // !defined(__CU_PROPERTY__)
