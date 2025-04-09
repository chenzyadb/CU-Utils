// CuPairList by chenzyadb@github.com
// Based on C++17 STL (GNUC)

#if !defined(__CU_PAIR_LIST__)
#define __CU_PAIR_LIST__ 1

#include <vector>
#include <string>
#include <algorithm>
#include <exception>
#include <functional>

namespace CU
{
    class PairListExcept : public std::exception
    {
        public:
            PairListExcept(const std::string &message) : message_(message) { }

            const char* what() const noexcept override
            {
                return message_.data();
            }

        private:
            const std::string message_;
    };

    template <typename _Key_Ty, typename _Val_Ty>
    class PairList
    {
        public:
            class Pair 
            {
                public:
                    Pair() : key_(), value_() { }

                    Pair(const _Key_Ty &key, const _Val_Ty &value) : key_(key), value_(value) { }

                    Pair(const Pair &other) : key_(other.key_), value_(other.value_) { }

                    Pair(Pair &&other) noexcept : key_(std::move(other.key_)), value_(std::move(other.value_)) { }

                    Pair &operator=(const Pair &other)
                    {
                        if (std::addressof(other) != this) {
                            key_ = other.key_;
                            value_ = other.value_;
                        }
                        return *this;
                    }

                    Pair &operator=(Pair &&other) noexcept
                    {
                        if (std::addressof(other) != this) {
                            key_ = std::move(other.key_);
                            value_ = std::move(other.value_);
                        }
                        return *this;
                    }

                    bool operator==(const Pair &other) const
                    {
                        if (std::addressof(other) == this) {
                            return true;
                        }
                        return (key_ == other.key_ && value_ == other.value_);
                    }

                    bool operator!=(const Pair &other) const
                    {
                        if (std::addressof(other) == this) {
                            return false;
                        }
                        return (key_ != other.key_ || value_ != other.value_);
                    }

                    _Key_Ty &key()
                    {
                        return key_;
                    }

                    const _Key_Ty &key() const
                    {
                        return key_;
                    }

                    _Val_Ty &value()
                    {
                        return value_;
                    }

                    const _Val_Ty &value() const
                    {
                        return value_;
                    }

                private:
                    _Key_Ty key_;
                    _Val_Ty value_;
            };

            typedef typename std::vector<Pair>::iterator iterator;
            typedef typename std::vector<Pair>::const_iterator const_iterator;

            PairList() : data_() { }

            PairList(const PairList &other) : data_(other.data_) { }

            PairList(PairList &&other) noexcept : data_(std::move(other.data_)) { }

            ~PairList() { }

            PairList &operator=(const PairList &other)
            {
                if (std::addressof(other) != this) {
                    data_ = other.data_;
                }
                return *this;
            }

            PairList &operator=(PairList &&other) noexcept
            {
                if (std::addressof(other) != this) {
                    data_ = std::move(other.data_);
                }
                return *this;
            }

            bool operator==(const PairList &other) const
            {
                if (std::addressof(other) == this) {
                    return true;
                }
                return (other.data_ == data_);
            }

            bool operator!=(const PairList &other) const
            {
                if (std::addressof(other) == this) {
                    return false;
                }
                return (other.data_ != data_);
            }

            _Val_Ty &operator[](const _Key_Ty &key)
            {
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    if (iter->key() == key) {
                        return iter->value();
                    }
                }
                data_.emplace_back(key, _Val_Ty());
                return data_.back().value();
            }

            _Key_Ty &operator()(const _Val_Ty &value)
            {
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    if (iter->value() == value) {
                        return iter->key();
                    }
                }
                data_.emplace_back(_Key_Ty(), value);
                return data_.back().key();
            }

            const _Val_Ty &atKey(const _Key_Ty &key) const
            {
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    if (iter->key() == key) {
                        return iter->value();
                    }
                }
                throw PairListExcept("Key not found");
            }

            const _Key_Ty &atValue(const _Val_Ty &value) const
            {
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    if (iter->value() == value) {
                        return iter->key();
                    }
                }
                throw PairListExcept("Value not found");
            }

            bool containsKey(const _Key_Ty &key) const
            {
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    if (iter->key() == key) {
                        return true;
                    }
                }
                return false;
            }

            bool containsValue(const _Val_Ty &value) const
            {
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    if (iter->value() == value) {
                        return true;
                    }
                }
                return false;
            }

            iterator begin()
            {
                return data_.begin();
            }

            const_iterator begin() const
            {
                return data_.begin();
            }

            iterator end()
            {
                return data_.end();
            }

            const_iterator end() const
            {
                return data_.end();
            }

            Pair &front()
            {
                return data_.front();
            }

            const Pair &front() const
            {
                return data_.front();
            }

            Pair &back()
            {
                return data_.back();
            }

            const Pair &back() const
            {
                return data_.back();
            }

            const_iterator findKey(const _Key_Ty &key) const
            {
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    if (iter->key() == key) {
                        return iter;
                    }
                }
                return data_.end();
            }

            const_iterator findValue(const _Val_Ty &value) const
            {
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    if (iter->value() == value) {
                        return iter;
                    }
                }
                return data_.end();
            }

            void add(const _Key_Ty &key, const _Val_Ty &value)
            {
                data_.emplace_back(key, value);
            }

            void add(const Pair &pair)
            {
                data_.emplace_back(pair);
            }

            void remove(const_iterator iter)
            {
                data_.erase(iter);
            }

            void remove(iterator iter)
            {
                data_.erase(iter);
            }

            void removeKey(const _Key_Ty &key)
            {
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    if (iter->key() == key) {
                        data_.erase(iter);
                        break;
                    }
                }
            }

            void removeValue(const _Val_Ty &value)
            {
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    if (iter->value() == value) {
                        data_.erase(iter);
                        break;
                    }
                }
            }

            std::vector<_Key_Ty> keys() const
            {
                std::vector<_Key_Ty> keysList{};
                keysList.reserve(data_.size());
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    keysList.emplace_back(iter->key());
                }
                return keysList;
            }

            std::vector<_Val_Ty> values() const
            {
                std::vector<_Val_Ty> valuesList{};
                valuesList.reserve(data_.size());
                for (auto iter = data_.begin(); iter < data_.end(); ++iter) {
                    valuesList.emplace_back(iter->value());
                }
                return valuesList;
            }

            void sort(std::function<bool(const Pair &, const Pair &)> compare_func = nullptr)
            {
                static const auto default_comp = [](const Pair &a, const Pair &b) -> bool {
                    return (a.key() < b.key());
                };

                if (!compare_func) {
                    std::sort(data_.begin(), data_.end(), default_comp);
                } else {
                    std::sort(data_.begin(), data_.end(), compare_func);
                }
            }

            void unique()
            {
                for (size_t pos = 0; pos < data_.size(); pos++) {
                    auto comp_pos = pos + 1;
                    while (comp_pos < data_.size()) {
                        if (data_[comp_pos] == data_[pos]) {
                            data_.erase(data_.begin() + comp_pos);
                        } else {
                            comp_pos++;
                        }
                    }
                }
            }

            void reverse()
            {
                std::reverse(data_.begin(), data_.end());
            }

            void clear()
            {
                data_.clear();
            }

            size_t size() const noexcept
            {
                return data_.size();
            }

            bool empty() const noexcept
            {
                return (data_.begin() == data_.end());
            }

        private:
            std::vector<Pair> data_;
    };
}

#endif // !defined(__CU_PAIR_LIST__)
