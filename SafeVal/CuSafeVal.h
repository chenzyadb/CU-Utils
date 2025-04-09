// CuSafeVal by chenzyadb@github.com
// Based on C++17 STL (GNUC)

#if !defined(__CU_SAFE_VAL__)
#define __CU_SAFE_VAL__ 1

#include <shared_mutex>
#include <mutex>
#include <functional>

namespace CU
{
    template <typename _Val_Ty>
    class SafeVal 
    {   
        public:
            SafeVal() : value_(), mutex_() { }

            SafeVal(const _Val_Ty &value) : value_(value), mutex_() { }

            SafeVal(_Val_Ty &&value) noexcept : value_(value), mutex_() { }

            SafeVal(const SafeVal &other) : value_(other.data()), mutex_() { }

            SafeVal(SafeVal &&other) noexcept : value_(other.data()), mutex_() { }

            SafeVal &operator=(const SafeVal &other)
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                if (std::addressof(other) != this) {
                    value_ = other.data();
                }
                return *this;
            }

            SafeVal &operator=(SafeVal &&other) noexcept
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                if (std::addressof(other) != this) {
                    value_ = other.data();
                }
                return *this;
            }

            _Val_Ty operator=(const _Val_Ty &value)
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                value_ = value;
                return value_;
            }

            _Val_Ty operator=(_Val_Ty &&value) noexcept
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                value_ = value;
                return value_;
            }

            operator _Val_Ty() const
            {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                return value_;
            }

            bool operator==(const SafeVal &other) const
            {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                if (std::addressof(other) == this) {
                    return true;
                }
                return (value_ == other.data());
            }

            bool operator!=(const SafeVal &other) const
            {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                if (std::addressof(other) == this) {
                    return false;
                }
                return (value_ != other.data());
            }

            _Val_Ty data() const
            {
                std::shared_lock<std::shared_mutex> lock(mutex_);
                return value_;
            }

            void use(const std::function<void(_Val_Ty &val)> &func)
            {
                if (func) {
                    std::unique_lock<std::shared_mutex> lock(mutex_);
                    func(value_);
                }
            }

            void use(const std::function<void(const _Val_Ty &val)> &func) const
            {
                if (func) {
                    std::shared_lock<std::shared_mutex> lock(mutex_);
                    func(value_);
                }
            }

        private:
            _Val_Ty value_;
            mutable std::shared_mutex mutex_;
    };
}

#endif // !defined(__CU_SAFE_VAL__)
