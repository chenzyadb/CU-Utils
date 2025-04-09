// CuJSONObject by chenzyadb@github.com
// Based on C++17 STL (MSVC)

#if !defined(_CU_JSONOBJECT_)
#define _CU_JSONOBJECT_ 1

#if defined(_MSC_VER)
#pragma warning(disable: 4996)
#pragma warning(disable: 4267)
#endif // defined(_MSC_VER)

#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <memory>
#include <exception>
#include <algorithm>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <cstring>

namespace CU
{
    class JSONExcept : public std::exception
    {
        public:
            JSONExcept(const std::string &message) : message_(message) { }

            const char* what() const noexcept override
            {
                return message_.c_str();
            }

        private:
            const std::string message_;
    };

    struct _JSON_String
    {
        char stack_buffer[128];
        char* heap_block;
        size_t length;
        size_t capacity;

        _JSON_String() noexcept : stack_buffer(), heap_block(nullptr), length(0), capacity(sizeof(stack_buffer)) { }

        _JSON_String(const _JSON_String &other) noexcept :
            stack_buffer(),
            heap_block(nullptr),
            length(0),
            capacity(sizeof(stack_buffer))
        {
            append(other);
        }

        _JSON_String(_JSON_String &&other) noexcept :
            stack_buffer(),
            heap_block(other.heap_block),
            length(other.length),
            capacity(other.capacity)
        {
            std::memcpy(stack_buffer, other.stack_buffer, sizeof(stack_buffer));
            other.heap_block = nullptr;
        }

        _JSON_String(const char* src) noexcept :
            stack_buffer(),
            heap_block(nullptr),
            length(0),
            capacity(sizeof(stack_buffer))
        {
            append(src);
        }

        ~_JSON_String() noexcept
        {
            if (heap_block != nullptr) {
                std::free(heap_block);
            }
        }

        void append(const _JSON_String &other) noexcept
        {
            auto new_len = length + other.length;
            if (new_len >= capacity) {
                resize(capacity + new_len);
            }
            std::memcpy((data() + length), other.data(), other.length);
            *(data() + new_len) = '\0';
            length = new_len;
        }

        void append(const char* src) noexcept
        {
            auto src_len = std::strlen(src);
            auto new_len = length + src_len;
            if (new_len >= capacity) {
                resize(capacity + new_len);
            }
            std::memcpy((data() + length), src, src_len);
            *(data() + new_len) = '\0';
            length = new_len;
        }

        void append(char ch) noexcept
        {
            if ((length + 1) >= capacity) {
                resize(capacity * 2);
            }
            *(data() + length) = ch;
            *(data() + length + 1) = '\0';
            length++;
        }

        void resize(size_t req_capacity) noexcept
        {
            if (req_capacity < capacity && length >= req_capacity) {
                shrink(req_capacity - 1);
            }
            if (req_capacity > sizeof(stack_buffer)) {
                auto new_block = reinterpret_cast<char*>(std::realloc(heap_block, req_capacity));
                if (new_block != nullptr) {
                    if (heap_block == nullptr) {
                        std::memcpy(new_block, stack_buffer, length);
                    }
                    *(new_block + length) = '\0';
                    heap_block = new_block;
                    capacity = req_capacity;
                }
            } else {
                capacity = sizeof(stack_buffer);
                if (heap_block != nullptr && length > 0) {
                    std::memcpy(stack_buffer, heap_block, length);
                    std::free(heap_block);
                }
                stack_buffer[length] = '\0';
            }
        }

        void shrink(size_t req_length) noexcept
        {
            if (req_length < length) {
                *(data() + req_length) = '\0';
                length = req_length;
            }
        }

        bool equals(const char* str) const noexcept
        {
            if (std::strlen(str) != length) {
                return false;
            }
            return (std::memcmp(data(), str, length) == 0);
        }

        char* data() noexcept
        {
            if (heap_block != nullptr) {
                return heap_block;
            }
            return stack_buffer;
        }

        const char* data() const noexcept
        {
            if (heap_block != nullptr) {
                return heap_block;
            }
            return stack_buffer;
        }

        void clear() noexcept
        {
            if (heap_block != nullptr) {
                std::free(heap_block);
                heap_block = nullptr;
            }
            stack_buffer[0] = '\0';
            length = 0;
            capacity = sizeof(stack_buffer);
        }
    };

    class JSONObject;
    class JSONArray;
    class JSONItem;

    namespace _JSON_Misc
    {
        inline char GetEscapeChar(const char &ch) noexcept
        {
            switch (ch) {
                case 'n':
                    return '\n';
                case 'r':
                    return '\r';
                case 't':
                    return '\t';
                case 'b':
                    return '\b';
                case 'f':
                    return '\f';
                case 'a':
                    return '\a';
                case 'v':
                    return '\v';
                default:
                    break;
            }
            return ch;
        }

        inline _JSON_String StringToJSONRaw(const std::string &str)
        {
            _JSON_String raw("\"");
            for (const auto &ch : str) {
                switch (ch) {
                    case '\\':
                        raw.append("\\\\");
                        break;
                    case '\"':
                        raw.append("\\\"");
                        break;
                    case '\n':
                        raw.append("\\n");
                        break;
                    case '\t':
                        raw.append("\\t");
                        break;
                    case '\r':
                        raw.append("\\r");
                        break;
                    case '\f':
                        raw.append("\\f");
                        break;
                    case '\a':
                        raw.append("\\a");
                        break;
                    case '\b':
                        raw.append("\\b");
                        break;
                    case '\v':
                        raw.append("\\v");
                        break;
                    case '/':
                        raw.append("\\/");
                        break;
                    default:
                        raw.append(ch);
                        break;
                }
            }
            raw.append('\"');
            return raw;
        }

        constexpr size_t npos = static_cast<size_t>(-1);

        inline size_t FindChar(const char* str, char ch, size_t start_pos = 0) noexcept
        {
            for (auto pos = start_pos; *(str + pos) != '\0'; pos++) {
                if (*(str + pos) == ch) {
                    return pos;
                }
            }
            return npos;
        }
    }

    class JSONItem
    {
        public:
            enum class ItemType : uint8_t {ITEM_NULL, BOOLEAN, INTEGER, LONG, DOUBLE, STRING, ARRAY, OBJECT};

            typedef decltype(nullptr) ItemNull;
            typedef std::variant<ItemNull, bool, int, int64_t, double, std::string, JSONArray*, JSONObject*> ItemValue;

            inline JSONItem();
            inline JSONItem(const _JSON_String &raw);
            inline JSONItem(ItemNull _null);
            inline JSONItem(bool value);
            inline JSONItem(int value);
            inline JSONItem(int64_t value);
            inline JSONItem(double value);
            inline JSONItem(const char* value);
            inline JSONItem(const std::string &value);
            inline JSONItem(const JSONArray &value);
            inline JSONItem(const JSONObject &value);
            inline JSONItem(const JSONItem &other);
            inline JSONItem(JSONItem &&other) noexcept;
            inline ~JSONItem();
            
            inline JSONItem &operator=(const JSONItem &other);
            inline JSONItem &operator=(JSONItem &&other) noexcept;
            inline bool operator==(const JSONItem &other) const;
            inline bool operator!=(const JSONItem &other) const;

            inline operator bool() const;
            inline operator int() const;
            inline operator int64_t() const;
            inline operator double() const;
            inline operator std::string() const;

            inline bool isNull() const;
            inline bool isBoolean() const;
            inline bool isInt() const;
            inline bool isLong() const;
            inline bool isDouble() const;
            inline bool isString() const;
            inline bool isArray() const;
            inline bool isObject() const;

            inline bool toBoolean() const;
            inline int toInt() const;
            inline int64_t toLong() const;
            inline double toDouble() const;
            inline std::string toString() const;
            inline JSONArray toArray() const;
            inline JSONObject toObject() const;
            inline std::string toRaw() const;

            inline void clear();
            inline size_t size() const;
            inline size_t hash() const;
            inline ItemType type() const;
            
        private:
            ItemType type_;
            ItemValue value_;
    };

    class JSONArray
    {
        public:
            typedef std::vector<JSONItem>::iterator iterator;
            typedef std::vector<JSONItem>::const_iterator const_iterator;

            inline JSONArray();
            inline JSONArray(size_t init_size);
            inline JSONArray(size_t init_size, const JSONItem &init_value);
            inline JSONArray(iterator begin_iter, iterator end_iter);
            inline JSONArray(std::string_view jsonText);
            inline JSONArray(const std::vector<JSONItem> &data);
            inline JSONArray(const JSONArray &other);
            inline JSONArray(JSONArray &&other) noexcept;
            inline ~JSONArray();
            
            inline JSONArray &operator=(const JSONArray &other);
            inline JSONArray &operator=(JSONArray &&other) noexcept;
            inline JSONArray &operator+=(const JSONArray &other);
            inline JSONArray operator+(const JSONArray &other) const;
            inline bool operator==(const JSONArray &other) const;
            inline bool operator!=(const JSONArray &other) const;
            inline JSONItem &operator[](size_t pos);

            inline const JSONItem &at(size_t pos) const;
            inline iterator find(const JSONItem &item);
            inline void add(const JSONItem &item);
            inline void remove(const JSONItem &item);
            inline void resize(size_t new_size);
            inline void clear();
            inline size_t size() const;
            inline size_t hash() const;
            inline bool empty() const;
            inline std::string toString() const;

            inline JSONItem &front();
            inline JSONItem &back();
            inline iterator begin();
            inline iterator end();
            inline const_iterator begin() const;
            inline const_iterator end() const;
            
        private:
            std::vector<JSONItem> data_;
    };

    class JSONObject
    {
        public:
            inline JSONObject();
            inline JSONObject(std::string_view jsonText);
            inline JSONObject(const JSONObject &other);
            inline JSONObject(JSONObject &&other) noexcept;
            inline JSONObject(std::unordered_map<std::string, JSONItem> &&data, std::vector<std::string> &&order) noexcept;
            inline ~JSONObject();

            inline JSONObject &operator=(const JSONObject &other);
            inline JSONObject &operator=(JSONObject &&other) noexcept;
            inline JSONObject &operator+=(const JSONObject &other);
            inline JSONObject operator+(const JSONObject &other) const;
            inline bool operator==(const JSONObject &other) const;
            inline bool operator!=(const JSONObject &other) const;
            inline JSONItem &operator[](const std::string &key);
            
            inline const JSONItem &at(const std::string &key) const;
            inline bool contains(const std::string &key) const;
            inline void add(const std::string &key, const JSONItem &value);
            inline void remove(const std::string &key);
            inline void clear();
            inline size_t size() const;
            inline size_t hash() const;
            inline bool empty() const;
            inline std::string toString() const;
            inline std::string toFormatedString() const;

            struct JSONPair
            {
                std::string key;
                JSONItem value;
            };
            inline std::vector<JSONPair> toPairs() const;

        private:
            std::unordered_map<std::string, JSONItem> data_;
            std::vector<std::string> order_;
    };

    inline JSONItem::JSONItem() :
        type_(ItemType::ITEM_NULL),
        value_(nullptr)
    { }

    inline JSONItem::JSONItem(const _JSON_String &raw) : type_(), value_()
    {
        auto len = raw.length;
        auto raw_data = raw.data();
        switch (*raw_data) {
            case '{':
                if (*(raw_data + len - 1) == '}') {
                    type_ = ItemType::OBJECT;
                    value_ = new JSONObject(raw_data);
                    return;
                }
                break;
            case '[':
                if (*(raw_data + len - 1) == ']') {
                    type_ = ItemType::ARRAY;
                    value_ = new JSONArray(raw_data);
                    return;
                }
                break;
            case '\"':
                if (*(raw_data + len - 1) == '\"') {
                    type_ = ItemType::STRING;
                    value_ = std::string(raw_data).substr(1, (len - 2));
                    return;
                }
                break;
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (_JSON_Misc::FindChar(raw_data, '.') != _JSON_Misc::npos) {
                    type_ = ItemType::DOUBLE;
                    value_ = std::strtod(raw_data, nullptr);
                    return;
                } else {
                    auto num = std::strtoll(raw_data, nullptr, 10);
                    if (num > std::numeric_limits<int>::max() || num < std::numeric_limits<int>::min()) {
                        type_ = ItemType::LONG;
                        value_ = static_cast<int64_t>(num);
                        return;
                    } else {
                        type_ = ItemType::INTEGER;
                        value_ = static_cast<int>(num);
                        return;
                    }
                }
                break;
            case 't':
                if (raw.equals("true")) {
                    type_ = ItemType::BOOLEAN;
                    value_ = true;
                    return;
                }
                break;
            case 'f':
                if (raw.equals("false")) {
                    type_ = ItemType::BOOLEAN;
                    value_ = false;
                    return;
                }
                break;
            case 'n':
                if (raw.equals("null")) {
                    type_ = ItemType::ITEM_NULL;
                    value_ = nullptr;
                    return;
                }
                break;
            default:
                break;
        }
        throw JSONExcept("Invalid JSONItem");
    }

    inline JSONItem::JSONItem(ItemNull _null) :
        type_(ItemType::ITEM_NULL),
        value_(nullptr)
    {
        (void)_null;
    }

    inline JSONItem::JSONItem(bool value) :
        type_(ItemType::BOOLEAN),
        value_(value)
    { }

    inline JSONItem::JSONItem(int value) :
        type_(ItemType::INTEGER),
        value_(value)
    { }

    inline JSONItem::JSONItem(int64_t value) :
        type_(ItemType::LONG),
        value_(value)
    { }

    inline JSONItem::JSONItem(double value) :
        type_(ItemType::DOUBLE),
        value_(value)
    { }

    inline JSONItem::JSONItem(const char* value) :
        type_(ItemType::STRING),
        value_(std::string(value))
    { }

    inline JSONItem::JSONItem(const std::string &value) :
        type_(ItemType::STRING),
        value_(value)
    { }

    inline JSONItem::JSONItem(const JSONArray &value) :
        type_(ItemType::ARRAY),
        value_(new JSONArray(value))
    { }

    inline JSONItem::JSONItem(const JSONObject &value) :
        type_(ItemType::OBJECT),
        value_(new JSONObject(value))
    { }

    inline JSONItem::JSONItem(const JSONItem &other) : type_(other.type_), value_()
    {
        const auto &other_value = other.value_;
        if (type_ == ItemType::ARRAY) {
            const auto &jsonArray = *(std::get<JSONArray*>(other_value));
            value_ = new JSONArray(jsonArray);
        } else if (type_ == ItemType::OBJECT) {
            const auto &jsonObject = *(std::get<JSONObject*>(other_value));
            value_ = new JSONObject(jsonObject);
        } else {
            value_ = other_value;
        }
    }

    inline JSONItem::JSONItem(JSONItem &&other) noexcept : type_(other.type_), value_(std::move(other.value_)) 
    {
        other.type_ = ItemType::ITEM_NULL;
        other.value_ = ItemValue();
    }

    inline JSONItem::~JSONItem()
    {
        if (type_ == ItemType::ARRAY) {
            delete std::get<JSONArray*>(value_);
        } else if (type_ == ItemType::OBJECT) {
            delete std::get<JSONObject*>(value_);
        }
    }

    inline JSONItem &JSONItem::operator=(const JSONItem &other)
    {
        if (std::addressof(other) != this) {
            clear();
            type_ = other.type_;
            const auto &other_value = other.value_;
            if (type_ == ItemType::ARRAY) {
                const auto &jsonArray = *(std::get<JSONArray*>(other_value));
                value_ = new JSONArray(jsonArray);
            } else if (type_ == ItemType::OBJECT) {
                const auto &jsonObject = *(std::get<JSONObject*>(other_value));
                value_ = new JSONObject(jsonObject);
            } else {
                value_ = other_value;
            }
        }
        return *this;
    }

    inline JSONItem &JSONItem::operator=(JSONItem &&other) noexcept
    {
        if (std::addressof(other) != this) {
            clear();
            type_ = other.type_;
            value_ = std::move(other.value_);

            other.type_ = ItemType::ITEM_NULL;
            other.value_ = ItemValue();
        }
        return *this;
    }

    inline bool JSONItem::operator==(const JSONItem &other) const
    {
        return (type_ == other.type_ && value_ == other.value_);
    }

    inline bool JSONItem::operator!=(const JSONItem &other) const
    {
        return (type_ != other.type_ || value_ != other.value_);
    }

    inline JSONItem::operator bool() const
    {
        return toBoolean();
    }

    inline JSONItem::operator int() const
    {
        return toInt();
    }

    inline JSONItem::operator int64_t() const
    {
        return toLong();
    }

    inline JSONItem::operator double() const
    {
        return toDouble();
    }

    inline JSONItem::operator std::string() const
    {
        return toString();
    }

    inline bool JSONItem::isNull() const
    {
        return (type_ == ItemType::ITEM_NULL);
    }

    inline bool JSONItem::isBoolean() const
    {
        return (type_ == ItemType::BOOLEAN);
    }

    inline bool JSONItem::isInt() const
    {
        return (type_ == ItemType::INTEGER);
    }

    inline bool JSONItem::isLong() const
    {
        return (type_ == ItemType::LONG);
    }

    inline bool JSONItem::isDouble() const
    {
        return (type_ == ItemType::DOUBLE);
    }

    inline bool JSONItem::isString() const
    {
        return (type_ == ItemType::STRING);
    }

    inline bool JSONItem::isArray() const
    {
        return (type_ == ItemType::ARRAY);
    }

    inline bool JSONItem::isObject() const
    {
        return (type_ == ItemType::OBJECT);
    }

    inline bool JSONItem::toBoolean() const
    {
        if (type_ != ItemType::BOOLEAN) {
            throw JSONExcept("Item is not of boolean type");
        }
        return std::get<bool>(value_);
    }

    inline int JSONItem::toInt() const
    {
        if (type_ != ItemType::INTEGER) {
            throw JSONExcept("Item is not of int type");
        }
        return std::get<int>(value_);
    }

    inline int64_t JSONItem::toLong() const
    {
        if (type_ != ItemType::LONG) {
            throw JSONExcept("Item is not of long type");
        }
        return std::get<int64_t>(value_);
    }

    inline double JSONItem::toDouble() const
    {
        if (type_ != ItemType::DOUBLE) {
            throw JSONExcept("Item is not of double type");
        }
        return std::get<double>(value_);
    }

    inline std::string JSONItem::toString() const
    {
        if (type_ != ItemType::STRING) {
            throw JSONExcept("Item is not of string type");
        }
        return std::get<std::string>(value_);
    }

    inline JSONArray JSONItem::toArray() const
    {
        if (type_ != ItemType::ARRAY) {
            throw JSONExcept("Item is not of array type");
        }
        return *(std::get<JSONArray*>(value_));
    }

    inline JSONObject JSONItem::toObject() const
    {
        if (type_ != ItemType::OBJECT) {
            throw JSONExcept("Item is not of object type");
        }
        return *(std::get<JSONObject*>(value_));
    }

    inline std::string JSONItem::toRaw() const
    {
        switch (type_) {
            case ItemType::ITEM_NULL:
                return "null";
            case ItemType::BOOLEAN:
                if (std::get<bool>(value_)) {
                    return "true";
                }
                return "false";
            case ItemType::INTEGER:
                return std::to_string(std::get<int>(value_));
            case ItemType::LONG:
                return std::to_string(std::get<int64_t>(value_));
            case ItemType::DOUBLE:
                return std::to_string(std::get<double>(value_));
            case ItemType::STRING:
                return _JSON_Misc::StringToJSONRaw(std::get<std::string>(value_)).data();
            case ItemType::ARRAY:
                return std::get<JSONArray*>(value_)->toString();
            case ItemType::OBJECT:
                return std::get<JSONObject*>(value_)->toString();
            default:
                break;
        }
        return {};
    }

    inline void JSONItem::clear()
    {
        if (type_ == ItemType::ARRAY) {
            delete std::get<JSONArray*>(value_);
        } else if (type_ == ItemType::OBJECT) {
            delete std::get<JSONObject*>(value_);
        }
        type_ = ItemType::ITEM_NULL;
        value_ = nullptr;
    }

    inline size_t JSONItem::size() const
    {
        switch (type_) {
            case ItemType::STRING:
                return std::get<std::string>(value_).size();
            case ItemType::ARRAY:
                return std::get<JSONArray*>(value_)->size();
            case ItemType::OBJECT:
                return std::get<JSONObject*>(value_)->size();
            default:
                break;
        }
        return 1;
    }

    inline size_t JSONItem::hash() const
    {
        switch (type_) {
            case ItemType::ITEM_NULL:
                break;
            case ItemType::BOOLEAN:
                {
                    std::hash<bool> hashVal{};
                    return hashVal(std::get<bool>(value_));
                }
            case ItemType::INTEGER:
                {
                    std::hash<int> hashVal{};
                    return hashVal(std::get<int>(value_));
                }
            case ItemType::LONG:
                {
                    std::hash<int64_t> hashVal{};
                    return hashVal(std::get<int64_t>(value_));
                }
            case ItemType::DOUBLE:
                {
                    std::hash<double> hashVal{};
                    return hashVal(std::get<double>(value_));
                }
            case ItemType::STRING:
                {
                    std::hash<std::string> hashVal{};
                    return hashVal(std::get<std::string>(value_));
                }
            case ItemType::ARRAY:
                return std::get<JSONArray*>(value_)->hash();
            case ItemType::OBJECT:
                return std::get<JSONObject*>(value_)->hash();
            default:
                break;
        }
        return 0;
    }

    inline JSONItem::ItemType JSONItem::type() const
    {
        return type_;
    }

    inline JSONArray::JSONArray() : data_() { }

    inline JSONArray::JSONArray(size_t init_size) : data_(init_size) { }

    inline JSONArray::JSONArray(size_t init_size, const JSONItem &init_value) : data_(init_size, init_value) { }

    inline JSONArray::JSONArray(iterator begin_iter, iterator end_iter) : data_(begin_iter, end_iter) { }

    inline JSONArray::JSONArray(std::string_view jsonText) : data_()
    {
        enum class ArrayIdx : uint8_t {NONE, ITEM_FRONT, ITEM_COMMON, ITEM_STRING, ITEM_ARRAY, ITEM_OBJECT, ITEM_BACK};
        size_t pos = 0;
        auto idx = ArrayIdx::NONE;
        uint32_t count = 0;
        _JSON_String content{};
        while (pos < jsonText.size()) {
            char ch = jsonText[pos];
            switch (ch) {
                case '[':
                    if (idx == ArrayIdx::NONE) {
                        idx = ArrayIdx::ITEM_FRONT;
                    } else if (idx == ArrayIdx::ITEM_FRONT) {
                        idx = ArrayIdx::ITEM_ARRAY;
                        count = 1;
                        content.append(ch);
                    } else if (idx == ArrayIdx::ITEM_ARRAY) {
                        count++;
                        content.append(ch);
                    } else if (idx == ArrayIdx::ITEM_STRING || idx == ArrayIdx::ITEM_OBJECT) {
                        content.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONArray Structure");
                    }
                    break;
                case ']':
                    if (idx == ArrayIdx::ITEM_BACK || idx == ArrayIdx::ITEM_COMMON) {
                        idx = ArrayIdx::NONE;
                    } else if (idx == ArrayIdx::ITEM_FRONT && data_.empty()) {
                        idx = ArrayIdx::NONE;
                    } else if (idx == ArrayIdx::ITEM_ARRAY) {
                        if (count > 0) {
                            count--;
                            content.append(ch);
                            if (count == 0) {
                                idx = ArrayIdx::ITEM_BACK;
                            }
                        } else {
                            throw JSONExcept("Invalid JSONArray Structure");
                        }
                    } else if (idx == ArrayIdx::ITEM_STRING || idx == ArrayIdx::ITEM_OBJECT) {
                        content.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONArray Structure");
                    }
                    break;
                case ',':
                    if (idx == ArrayIdx::ITEM_BACK || idx == ArrayIdx::ITEM_COMMON) {
                        idx = ArrayIdx::ITEM_FRONT;
                    } else if (idx == ArrayIdx::ITEM_STRING || idx == ArrayIdx::ITEM_ARRAY || idx == ArrayIdx::ITEM_OBJECT) {
                        content.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONArray Structure");
                    }
                    break;
                case '{':
                    if (idx == ArrayIdx::ITEM_FRONT) {
                        idx = ArrayIdx::ITEM_OBJECT;
                        count = 1;
                        content.append(ch);
                    } else if (idx == ArrayIdx::ITEM_OBJECT) {
                        count++;
                        content.append(ch);
                    } else if (idx == ArrayIdx::ITEM_STRING || idx == ArrayIdx::ITEM_ARRAY) {
                        content.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONArray Structure");
                    }
                    break;
                case '}':
                    if (idx == ArrayIdx::ITEM_OBJECT) {
                        if (count > 0) {
                            count--;
                            content.append(ch);
                            if (count == 0) {
                                idx = ArrayIdx::ITEM_BACK;
                            }
                        } else {
                            throw JSONExcept("Invalid JSONArray Structure");
                        }
                    } else if (idx == ArrayIdx::ITEM_STRING || idx == ArrayIdx::ITEM_ARRAY) {
                        content.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONArray Structure");
                    }
                    break;
                case '\"':
                    if (idx == ArrayIdx::ITEM_FRONT) {
                        idx = ArrayIdx::ITEM_STRING;
                        content.append(ch);
                    } else if (idx == ArrayIdx::ITEM_STRING) {
                        idx = ArrayIdx::ITEM_BACK;
                        content.append(ch);
                    } else if (idx == ArrayIdx::ITEM_ARRAY || idx == ArrayIdx::ITEM_OBJECT) {
                        content.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONArray Structure");
                    }
                    break;
                case ' ':
                    if (idx == ArrayIdx::ITEM_STRING || idx == ArrayIdx::ITEM_ARRAY || idx == ArrayIdx::ITEM_OBJECT) {
                        content.append(ch);
                    }
                    break;
                case '\n':
                case '\t':
                case '\r':
                case '\f':
                case '\a':
                case '\b':
                case '\v':
                    break;
                case '\\':
                    if (idx == ArrayIdx::ITEM_STRING) {
                        if ((pos + 1) >= jsonText.size()) {
                            throw JSONExcept("Invalid JSONArray Structure");
                        }
                        pos++;
                        content.append(_JSON_Misc::GetEscapeChar(jsonText[pos]));
                    } else if (idx == ArrayIdx::ITEM_ARRAY || idx == ArrayIdx::ITEM_OBJECT) {
                        content.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONArray Structure");
                    }
                    break;
                default:
                    if (idx == ArrayIdx::ITEM_FRONT) {
                        idx = ArrayIdx::ITEM_COMMON;
                        content.append(ch);
                    } else if (idx != ArrayIdx::NONE && idx != ArrayIdx::ITEM_BACK) {
                        content.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONArray Structure");
                    }
                    break;
            }
            if ((idx == ArrayIdx::ITEM_FRONT || idx == ArrayIdx::NONE) && content.length > 0) {
                data_.emplace_back(JSONItem(content));
                content.shrink(0);
            }
            pos++;
        }
        if (idx != ArrayIdx::NONE) {
            throw JSONExcept("Invalid JSONArray Structure");
        }
    }

    inline JSONArray::JSONArray(const std::vector<JSONItem> &data) : data_(data) { }

    inline JSONArray::JSONArray(const JSONArray &other) : data_(other.data_) { }

    inline JSONArray::JSONArray(JSONArray &&other) noexcept : data_(std::move(other.data_)) { }

    inline JSONArray::~JSONArray() { }

    inline JSONArray &JSONArray::operator=(const JSONArray &other)
    {
        if (std::addressof(other) != this) {
            data_ = other.data_;
        }
        return *this;
    }

    inline JSONArray &JSONArray::operator=(JSONArray &&other) noexcept
    {
        if (std::addressof(other) != this) {
            data_ = std::move(other.data_);
        }
        return *this;
    }

    inline JSONArray &JSONArray::operator+=(const JSONArray &other)
    {
        if (std::addressof(other) != this) {
            const auto &other_data = other.data_;
            for (auto iter = other_data.begin(); iter < other_data.end(); iter++) {
                data_.emplace_back(*iter);
            }
        }
        return *this;
    }

    inline JSONArray JSONArray::operator+(const JSONArray &other) const
    {
        auto merged_data = data_;
        if (std::addressof(other) != this) {
            const auto &other_data = other.data_;
            for (auto iter = other_data.begin(); iter < other_data.end(); iter++) {
                merged_data.emplace_back(*iter);
            }
        }
        return JSONArray(merged_data);
    }

    inline bool JSONArray::operator==(const JSONArray &other) const
    {
        return (data_ == other.data_);
    }

    inline bool JSONArray::operator!=(const JSONArray &other) const
    {
        return (data_ != other.data_);
    }

    inline JSONItem &JSONArray::operator[](size_t pos)
    {
        if (pos >= data_.size()) {
            throw JSONExcept("Position out of bounds");
        }
        return data_.at(pos);
    }

    inline const JSONItem &JSONArray::at(size_t pos) const
    {
        if (pos >= data_.size()) {
            throw JSONExcept("Position out of bounds");
        }
        return data_.at(pos);
    }

    inline JSONArray::iterator JSONArray::find(const JSONItem &item)
    {
        if (data_.begin() == data_.end()) {
            return data_.end();
        }
        return std::find(data_.begin(), data_.end(), item);
    }

    inline void JSONArray::add(const JSONItem &item)
    {
        data_.emplace_back(item);
    }

    inline void JSONArray::remove(const JSONItem &item)
    {
        auto iter = std::find(data_.begin(), data_.end(), item);
        if (iter == data_.end()) {
            throw JSONExcept("Item not found");
        }
        data_.erase(iter);
    }

    inline void JSONArray::resize(size_t new_size)
    {
        data_.resize(new_size);
    }

    inline void JSONArray::clear()
    {
        data_.clear();
    }

    inline size_t JSONArray::size() const
    {
        return data_.size();
    }

    inline size_t JSONArray::hash() const
    {
        size_t hashVal = data_.size();
        for (const auto &item : data_) {
            hashVal ^= item.hash() + 2654435769 + (hashVal << 6) + (hashVal >> 2);
        }
        return hashVal;
    }

    inline bool JSONArray::empty() const
    {
        return (data_.begin() == data_.end());
    }

    inline std::string JSONArray::toString() const
    {
        if (data_.begin() == data_.end()) {
            std::string JSONText("[]");
            return JSONText;
        } else if ((data_.begin() + 1) == data_.end()) {
            auto JSONText = std::string("[") + data_.front().toRaw() + "]";
            return JSONText;
        }
        std::string JSONText("[");
        for (auto iter = data_.begin(); iter < (data_.end() - 1); iter++) {
            JSONText += iter->toRaw() + ",";
        }
        JSONText += data_.back().toRaw() + "]";
        return JSONText;
    }

    inline JSONItem &JSONArray::front()
    {
        return data_.front();
    }

    inline JSONItem &JSONArray::back()
    {
        return data_.back();
    }

    inline JSONArray::iterator JSONArray::begin()
    {
        return data_.begin();
    }

    inline JSONArray::iterator JSONArray::end()
    {
        return data_.end();
    }

    inline JSONArray::const_iterator JSONArray::begin() const
    {
        return data_.begin();
    }

    inline JSONArray::const_iterator JSONArray::end() const
    {
        return data_.end();
    }

    inline JSONObject::JSONObject() : data_(), order_() { }

    inline JSONObject::JSONObject(std::string_view jsonText) : data_(), order_()
    {
        enum class ObjectIdx : uint8_t
        {NONE, KEY_FRONT, KEY_CONTENT, KEY_BACK, VALUE_FRONT, VALUE_COMMON, VALUE_STRING, VALUE_ARRAY, VALUE_OBJECT, VALUE_BACK};
        size_t pos = 0;
        auto idx = ObjectIdx::NONE;
        uint32_t count = 0;
        _JSON_String key{}, value{};
        while (pos < jsonText.size()) {
            char ch = jsonText[pos];
            switch (ch) {
                case '{':
                    if (idx == ObjectIdx::NONE) {
                        idx = ObjectIdx::KEY_FRONT;
                    } else if (idx == ObjectIdx::VALUE_FRONT) {
                        idx = ObjectIdx::VALUE_OBJECT;
                        count = 1;
                        value.append(ch);
                    } else if (idx == ObjectIdx::VALUE_OBJECT) {
                        count++;
                        value.append(ch);
                    } else if (idx == ObjectIdx::VALUE_STRING || idx == ObjectIdx::VALUE_ARRAY) {
                        value.append(ch);
                    } else if (idx == ObjectIdx::KEY_CONTENT) {
                        key.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONObject Structure");
                    }
                    break;
                case '}':
                    if (idx == ObjectIdx::KEY_FRONT && data_.empty()) {
                        idx = ObjectIdx::NONE;
                    } else if (idx == ObjectIdx::VALUE_OBJECT) {
                        if (count > 0) {
                            count--;
                            value.append(ch);
                            if (count == 0) {
                                idx = ObjectIdx::VALUE_BACK;
                            }
                        } else {
                            throw JSONExcept("Invalid JSONObject Structure");
                        }
                    } else if (idx == ObjectIdx::VALUE_BACK || idx == ObjectIdx::VALUE_COMMON) {
                        idx = ObjectIdx::NONE;
                    } else if (idx == ObjectIdx::VALUE_STRING || idx == ObjectIdx::VALUE_ARRAY) {
                        value.append(ch);
                    } else if (idx == ObjectIdx::KEY_CONTENT) {
                        key.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONObject Structure");
                    }
                    break;
                case '\"':
                    if (idx == ObjectIdx::KEY_FRONT) {
                        idx = ObjectIdx::KEY_CONTENT;
                    } else if (idx == ObjectIdx::KEY_CONTENT) {
                        idx = ObjectIdx::KEY_BACK;
                    } else if (idx == ObjectIdx::VALUE_FRONT) {
                        idx = ObjectIdx::VALUE_STRING;
                        value.append(ch);
                    } else if (idx == ObjectIdx::VALUE_STRING) {
                        idx = ObjectIdx::VALUE_BACK;
                        value.append(ch);
                    } else if (idx == ObjectIdx::VALUE_ARRAY || idx == ObjectIdx::VALUE_OBJECT) {
                        value.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONObject Structure");
                    }
                    break;
                case ':':
                    if (idx == ObjectIdx::KEY_BACK) {
                        idx = ObjectIdx::VALUE_FRONT;
                    } else if (idx == ObjectIdx::VALUE_STRING || idx == ObjectIdx::VALUE_ARRAY || idx == ObjectIdx::VALUE_OBJECT) {
                        value.append(ch);
                    } else if (idx == ObjectIdx::KEY_CONTENT) {
                        key.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONObject Structure");
                    }
                    break;
                case ',':
                    if (idx == ObjectIdx::VALUE_COMMON || idx == ObjectIdx::VALUE_BACK) {
                        idx = ObjectIdx::KEY_FRONT;
                    } else if (idx == ObjectIdx::VALUE_STRING || idx == ObjectIdx::VALUE_ARRAY || idx == ObjectIdx::VALUE_OBJECT) {
                        value.append(ch);
                    } else if (idx == ObjectIdx::KEY_CONTENT) {
                        key.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONObject Structure");
                    }
                    break;
                case '[':
                    if (idx == ObjectIdx::VALUE_FRONT) {
                        idx = ObjectIdx::VALUE_ARRAY;
                        count = 1;
                        value.append(ch);
                    } else if (idx == ObjectIdx::VALUE_ARRAY) {
                        count++;
                        value.append(ch);
                    } else if (idx == ObjectIdx::VALUE_STRING || idx == ObjectIdx::VALUE_OBJECT) {
                        value.append(ch);
                    } else if (idx == ObjectIdx::KEY_CONTENT) {
                        key.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONObject Structure");
                    }
                    break;
                case ']':
                    if (idx == ObjectIdx::VALUE_ARRAY) {
                        if (count > 0) {
                            count--;
                            value.append(ch);
                            if (count == 0) {
                                idx = ObjectIdx::VALUE_BACK;
                            }
                        } else {
                            throw JSONExcept("Invalid JSONObject Structure");
                        }
                    } else if (idx == ObjectIdx::VALUE_STRING || idx == ObjectIdx::VALUE_OBJECT) {
                        value.append(ch);
                    } else if (idx == ObjectIdx::KEY_CONTENT) {
                        key.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONObject Structure");
                    }
                    break;
                case ' ':
                    if (idx == ObjectIdx::VALUE_STRING || idx == ObjectIdx::VALUE_ARRAY || idx == ObjectIdx::VALUE_OBJECT) {
                        value.append(ch);
                    } else if (idx == ObjectIdx::KEY_CONTENT) {
                        key.append(ch);
                    }
                    break;
                case '\n':
                case '\t':
                case '\r':
                case '\f':
                case '\a':
                case '\b':
                case '\v':
                    break;
                case '\\':
                    if (idx == ObjectIdx::KEY_CONTENT) {
                        if ((pos + 1) >= jsonText.size()) {
                            throw JSONExcept("Invalid JSONObject Structure");
                        }
                        pos++;
                        key.append(_JSON_Misc::GetEscapeChar(jsonText[pos]));
                    } else if (idx == ObjectIdx::VALUE_STRING) {
                        if ((pos + 1) >= jsonText.size()) {
                            throw JSONExcept("Invalid JSONObject Structure");
                        }
                        pos++;
                        value.append(_JSON_Misc::GetEscapeChar(jsonText[pos]));
                    } else if (idx == ObjectIdx::VALUE_ARRAY || idx == ObjectIdx::VALUE_OBJECT) {
                        value.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONObject Structure");
                    }
                    break;
                default:
                    if (idx == ObjectIdx::VALUE_FRONT) {
                        idx = ObjectIdx::VALUE_COMMON;
                        value.append(ch);
                    } else if (idx == ObjectIdx::KEY_CONTENT) {
                        key.append(ch);
                    } else if (idx == ObjectIdx::VALUE_COMMON || idx == ObjectIdx::VALUE_STRING ||
                        idx == ObjectIdx::VALUE_ARRAY || idx == ObjectIdx::VALUE_OBJECT
                    ) {
                        value.append(ch);
                    } else {
                        throw JSONExcept("Invalid JSONObject Structure");
                    }
                    break;
            }
            if ((idx == ObjectIdx::KEY_FRONT || idx == ObjectIdx::NONE) && key.length > 0 && value.length > 0) {
                order_.emplace_back(key.data());
                data_.emplace(key.data(), JSONItem(value));
                key.shrink(0);
                value.shrink(0);
            }
            pos++;
        }
        if (idx != ObjectIdx::NONE) {
            throw JSONExcept("Invalid JSONObject Structure");
        }
    }

    inline JSONObject::JSONObject(const JSONObject &other) :
        data_(other.data_),
        order_(other.order_)
    { }

    inline JSONObject::JSONObject(JSONObject &&other) noexcept :
        data_(std::move(other.data_)),
        order_(std::move(other.order_))
    { }

    inline JSONObject::JSONObject(std::unordered_map<std::string, JSONItem> &&data, std::vector<std::string> &&order) noexcept :
        data_(data),
        order_(order)
    { }

    inline JSONObject::~JSONObject() { }

    inline JSONObject &JSONObject::operator=(const JSONObject &other)
    {
        if (std::addressof(other) != this) {
            data_ = other.data_;
            order_ = other.order_;
        }
        return *this;
    }

    inline JSONObject &JSONObject::operator=(JSONObject &&other) noexcept
    {
        if (std::addressof(other) != this) {
            data_ = std::move(other.data_);
            order_ = std::move(other.order_);
        }
        return *this;
    }

    inline JSONObject &JSONObject::operator+=(const JSONObject &other)
    {
        if (std::addressof(other) != this) {
            const auto &other_data = other.data_;
            const auto &other_order = other.order_;
            for (const auto &key : other_order) {
                if (data_.count(key) == 0) {
                    order_.emplace_back(key);
                    data_.emplace(key, other_data.at(key));
                } else {
                    data_[key] = other_data.at(key);
                }
            }
        }
        return *this;
    }

    inline JSONObject JSONObject::operator+(const JSONObject &other) const
    {
        std::unordered_map<std::string, JSONItem> merged_data(other.data_);
        std::vector<std::string> merged_order(other.order_);
        for (const auto &key : order_) {
            if (merged_data.count(key) == 1) {
                merged_data[key] = data_.at(key);
            } else {
                merged_order.emplace_back(key);
                merged_data.emplace(key, data_.at(key));
            }
        }
        return JSONObject(std::move(merged_data), std::move(merged_order));
    }

    inline bool JSONObject::operator==(const JSONObject &other) const
    {
        return (data_ == other.data_ && order_ == other.order_);
    }

    inline bool JSONObject::operator!=(const JSONObject &other) const
    {
        return (data_ != other.data_ || order_ != other.order_);
    }

    inline bool JSONObject::contains(const std::string &key) const
    {
        return (data_.count(key) == 1);
    }

    inline JSONItem &JSONObject::operator[](const std::string &key)
    {
        if (data_.count(key) == 0) {
            order_.emplace_back(key);
        }
        return data_[key];
    }

    inline const JSONItem &JSONObject::at(const std::string &key) const
    {
        auto iter = data_.find(key);
        if (iter == data_.end()) {
            throw JSONExcept("Key not found");
        }
        return iter->second;
    }

    inline void JSONObject::add(const std::string &key, const JSONItem &value)
    {
        if (data_.count(key) == 0) {
            order_.emplace_back(key);
            data_.emplace(key, value);
        } else {
            data_[key] = value;
        }
    }

    inline void JSONObject::remove(const std::string &key)
    {
        auto iter = std::find(order_.begin(), order_.end(), key);
        if (iter == order_.end()) {
            throw JSONExcept("Key not found");
        }
        data_.erase(key);
        order_.erase(iter);
    }

    inline void JSONObject::clear()
    {
        data_.clear();
        order_.clear();
    }

    inline size_t JSONObject::size() const
    {
        return data_.size();
    }

    inline size_t JSONObject::hash() const
    {
        size_t hashVal = data_.size();
        for (auto iter = data_.begin(); iter != data_.end(); iter++) {
            hashVal ^= (iter->second).hash() + 2654435769 + (hashVal << 6) + (hashVal >> 2);
        }
        return hashVal;
    }

    inline bool JSONObject::empty() const
    {
        return (data_.begin() == data_.end());
    }

    inline std::string JSONObject::toString() const
    {
        if (order_.size() == 1) {
            _JSON_String jsonText("{");
            jsonText.append(_JSON_Misc::StringToJSONRaw(order_.front()));
            jsonText.append(':');
            jsonText.append(data_.at(order_.front()).toRaw().data());
            jsonText.append('}');
            return jsonText.data();
        } else if (order_.size() > 1) {
            _JSON_String jsonText("{");
            for (auto iter = order_.begin(); iter < (order_.end() - 1); iter++) {
                const auto &key = *iter;
                jsonText.append(_JSON_Misc::StringToJSONRaw(key));
                jsonText.append(':');
                jsonText.append(data_.at(key).toRaw().data());
                jsonText.append(',');
            }
            jsonText.append(_JSON_Misc::StringToJSONRaw(order_.back()));
            jsonText.append(':');
            jsonText.append(data_.at(order_.back()).toRaw().data());
            jsonText.append('}');
            return jsonText.data();
        }
        return "{}";
    }

    inline std::string JSONObject::toFormatedString() const
    {
        if (order_.size() == 1) {
            _JSON_String jsonText("{\n");
            jsonText.append(_JSON_Misc::StringToJSONRaw(order_.front()));
            jsonText.append(": ");
            jsonText.append(data_.at(order_.front()).toRaw().data());
            jsonText.append("\n }");
            return jsonText.data();
        } else if (order_.size() > 1) {
            _JSON_String jsonText("{\n");
            for (auto iter = order_.begin(); iter < (order_.end() - 1); iter++) {
                const auto &key = *iter;
                jsonText.append("  ");
                jsonText.append(_JSON_Misc::StringToJSONRaw(key));
                jsonText.append(": ");
                jsonText.append(data_.at(key).toRaw().data());
                jsonText.append(",\n");
            }
            jsonText.append("  ");
            jsonText.append(_JSON_Misc::StringToJSONRaw(order_.back()));
            jsonText.append(": ");
            jsonText.append(data_.at(order_.back()).toRaw().data());
            jsonText.append("\n}");
            return jsonText.data();
        }
        return "{ }";
    }

    inline std::vector<JSONObject::JSONPair> JSONObject::toPairs() const
    {
        std::vector<JSONPair> pairs{};
        pairs.resize(order_.size());
        for (size_t pos = 0; pos < pairs.size(); pos++) {
            pairs[pos].key = order_[pos];
            pairs[pos].value = data_.at(pairs[pos].key);
        }
        return pairs;
    }

    namespace JSONBinary
    {
        typedef uint32_t pos_t;
        typedef int8_t byte_t;

        constexpr pos_t npos = static_cast<pos_t>(-1);

        inline void* _Move_Ptr(void* ptr, pos_t pos) noexcept
        {
            return reinterpret_cast<void*>(reinterpret_cast<byte_t*>(ptr) + pos);
        }

        inline const void* _Move_Ptr(const void* ptr, pos_t pos) noexcept
        {
            return reinterpret_cast<const void*>(reinterpret_cast<const byte_t*>(ptr) + pos);
        }

        class _Binary_Container
        {
            public:
                _Binary_Container() noexcept : 
                    block_(reinterpret_cast<byte_t*>(std::malloc(128))), 
                    capacity_(128), 
                    size_(0)
                { }

                ~_Binary_Container() noexcept
                {
                    if (block_ != nullptr) {
                        std::free(block_);
                    }
                }

                void resize(pos_t req_capacity) noexcept
                {
                    auto new_block = reinterpret_cast<byte_t*>(std::realloc(block_, req_capacity));
                    if (new_block != nullptr) {
                        block_ = new_block;
                        capacity_ = req_capacity;
                    }
                    if (size_ > capacity_) {
                        size_ = capacity_;
                    }
                }

                void add(const void* data, pos_t data_size) noexcept
                {
                    if ((size_ + data_size) > capacity_) {
                        resize(size_ * 2 + data_size);
                    }
                    auto dst = _Move_Ptr(block_, size_);
                    if (dst != nullptr && data != nullptr) {
                        std::memcpy(dst, data, data_size);
                        size_ += data_size;
                    }
                }

                byte_t* data() noexcept
                {
                    return block_;
                }

                byte_t* dump() noexcept
                {
                    auto dump_data = block_;
                    block_ = nullptr;
                    capacity_ = 0;
                    size_ = 0;
                    return dump_data;
                }

                pos_t size() const noexcept
                {
                    return size_;
                }

            private:
                byte_t* block_;
                pos_t capacity_;
                pos_t size_;
        };

        inline pos_t _GetBinarySize(const byte_t* binary) noexcept
        {
            return *reinterpret_cast<const pos_t*>(binary);
        }

        inline void _DeleteBinary(byte_t* binary) noexcept
        {
            if (binary != nullptr) {
                std::free(binary);
            }
        }

        inline byte_t* _ObjectToBinary(const JSONObject &object);

        inline CU::JSONObject _BinaryToObject(const byte_t* binary);

        inline byte_t* _ArrayToBinary(const JSONArray &array)
        {
            static const auto addBlock = [](_Binary_Container &container, const JSONItem &item) {
                switch (item.type()) {
                    case JSONItem::ItemType::ITEM_NULL:
                        {
                            auto type = JSONItem::ItemType::ITEM_NULL;
                            pos_t block_size = sizeof(type) + sizeof(pos_t);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                        }
                        break;
                    case JSONItem::ItemType::BOOLEAN:
                        {
                            bool data = item;
                            auto type = JSONItem::ItemType::BOOLEAN;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + sizeof(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(std::addressof(data), sizeof(data));
                        }
                        break;
                    case JSONItem::ItemType::INTEGER:
                        {
                            int data = item;
                            auto type = JSONItem::ItemType::INTEGER;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + sizeof(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(std::addressof(data), sizeof(data));
                        }
                        break;
                    case JSONItem::ItemType::LONG:
                        {
                            int64_t data = item;
                            auto type = JSONItem::ItemType::LONG;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + sizeof(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(std::addressof(data), sizeof(data));
                        }
                        break;
                    case JSONItem::ItemType::DOUBLE:
                        {
                            double data = item;
                            auto type = JSONItem::ItemType::DOUBLE;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + sizeof(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(std::addressof(data), sizeof(data));
                        }
                        break;
                    case JSONItem::ItemType::STRING:
                        {
                            std::string data = item;
                            auto type = JSONItem::ItemType::STRING;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + data.length() + 1;
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(data.data(), (data.length() + 1));
                        }
                        break;
                    case JSONItem::ItemType::ARRAY:
                        {
                            auto data = _ArrayToBinary(item.toArray());
                            auto type = JSONItem::ItemType::ARRAY;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + _GetBinarySize(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(data, _GetBinarySize(data));
                            _DeleteBinary(data);
                        }
                        break;
                    case JSONItem::ItemType::OBJECT:
                        {
                            auto data = _ObjectToBinary(item.toObject());
                            auto type = JSONItem::ItemType::OBJECT;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + _GetBinarySize(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(data, _GetBinarySize(data));
                            _DeleteBinary(data);
                        }
                        break;
                    default:
                        break;
                }
            };

            // Array Binary Structure: [binary_size][[block_size][type][data]]...[end_block]
            _Binary_Container container{};
            {
                pos_t binary_size = sizeof(pos_t);
                container.add(std::addressof(binary_size), sizeof(binary_size));
            }
            for (const auto &item : array) {
                addBlock(container, item);
            }
            container.add(std::addressof(npos), sizeof(npos));
            *reinterpret_cast<pos_t*>(container.data()) = container.size();
            return container.dump();
        }

        inline CU::JSONArray _BinaryToArray(const byte_t* binary)
        {
            static const auto addItem = [](JSONArray &array, const byte_t* binary, pos_t block_offset) -> pos_t {
                auto block_size = *reinterpret_cast<const pos_t*>(_Move_Ptr(binary, block_offset));
                if (block_size == npos) {
                    return npos;
                }
                auto type = *reinterpret_cast<const JSONItem::ItemType*>(_Move_Ptr(binary, (block_offset + sizeof(block_size))));
                auto data = _Move_Ptr(binary, (block_offset + sizeof(block_size) + sizeof(type)));
                switch (type) {
                    case JSONItem::ItemType::ITEM_NULL:
                        array.add(nullptr);
                        break;
                    case JSONItem::ItemType::BOOLEAN:
                        array.add(*reinterpret_cast<const bool*>(data));
                        break;
                    case JSONItem::ItemType::INTEGER:
                        array.add(*reinterpret_cast<const int*>(data));
                        break;
                    case JSONItem::ItemType::LONG:
                        array.add(*reinterpret_cast<const int64_t*>(data));
                        break;
                    case JSONItem::ItemType::DOUBLE:
                        array.add(*reinterpret_cast<const double*>(data));
                        break;
                    case JSONItem::ItemType::STRING:
                        array.add(reinterpret_cast<const char*>(data));
                        break;
                    case JSONItem::ItemType::ARRAY:
                        array.add(_BinaryToArray(reinterpret_cast<const byte_t*>(data)));
                        break;
                    case JSONItem::ItemType::OBJECT:
                        array.add(_BinaryToObject(reinterpret_cast<const byte_t*>(data)));
                        break;
                    default:
                        break;
                }
                return (block_offset + block_size);
            };

            JSONArray array{};
            pos_t size = _GetBinarySize(binary), offset = sizeof(pos_t);
            while (offset < size) {
                offset = addItem(array, binary, offset);
            }
            return array;
        }

        inline void SaveArray(const std::string &path, const JSONArray &array)
        {
            auto fp = std::fopen(path.c_str(), "wb");
            if (fp != nullptr) {
                auto binary = _ArrayToBinary(array);
                if (binary != nullptr) {
                    std::fwrite(binary, sizeof(byte_t), _GetBinarySize(binary), fp);
                    std::fflush(fp);
                    _DeleteBinary(binary);
                }
                std::fclose(fp);
            }
        }

        inline CU::JSONArray OpenArray(const std::string &path)
        {
            CU::JSONArray array{};
            auto fp = std::fopen(path.c_str(), "rb");
            if (fp != nullptr) {
                pos_t binary_size = 0;
                std::fread(std::addressof(binary_size), sizeof(binary_size), 1, fp);
                auto buffer = reinterpret_cast<byte_t*>(std::malloc(binary_size));
                if (buffer != nullptr) {
                    std::rewind(fp);
                    std::fread(buffer, sizeof(byte_t), binary_size, fp);
                    array = _BinaryToArray(buffer);
                    _DeleteBinary(buffer);
                }
                std::fclose(fp);
            }
            return array;
        }

        inline byte_t* _ObjectToBinary(const JSONObject &object)
        {
            static const auto addBlock = [](_Binary_Container &container, const std::string &key, const JSONItem &item) {
                switch (item.type()) {
                    case JSONItem::ItemType::ITEM_NULL:
                        {
                            auto type = JSONItem::ItemType::ITEM_NULL;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + key.length() + 1;
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(key.data(), (key.length() + 1));
                        }
                        break;
                    case JSONItem::ItemType::BOOLEAN:
                        {
                            bool data = item;
                            auto type = JSONItem::ItemType::BOOLEAN;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + key.length() + 1 + sizeof(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(key.data(), (key.length() + 1));
                            container.add(std::addressof(data), sizeof(data));
                        }
                        break;
                    case JSONItem::ItemType::INTEGER:
                        {
                            int data = item;
                            auto type = JSONItem::ItemType::INTEGER;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + key.length() + 1 + sizeof(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(key.data(), (key.length() + 1));
                            container.add(std::addressof(data), sizeof(data));
                        }
                        break;
                    case JSONItem::ItemType::LONG:
                        {
                            int64_t data = item;
                            auto type = JSONItem::ItemType::LONG;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + key.length() + 1 + sizeof(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(key.data(), (key.length() + 1));
                            container.add(std::addressof(data), sizeof(data));
                        }
                        break;
                    case JSONItem::ItemType::DOUBLE:
                        {
                            double data = item;
                            auto type = JSONItem::ItemType::DOUBLE;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + key.length() + 1 + sizeof(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(key.data(), (key.length() + 1));
                            container.add(std::addressof(data), sizeof(data));
                        }
                        break;
                    case JSONItem::ItemType::STRING:
                        {
                            std::string data = item;
                            auto type = JSONItem::ItemType::STRING;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + key.length() + 1 + data.length() + 1;
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(key.data(), (key.length() + 1));
                            container.add(data.data(), (data.length() + 1));
                        }
                        break;
                    case JSONItem::ItemType::ARRAY:
                        {
                            auto data = _ArrayToBinary(item.toArray());
                            auto type = JSONItem::ItemType::ARRAY;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + key.length() + 1 + _GetBinarySize(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(key.data(), (key.length() + 1));
                            container.add(data, _GetBinarySize(data));
                            _DeleteBinary(data);
                        }
                        break;
                    case JSONItem::ItemType::OBJECT:
                        {
                            auto data = _ObjectToBinary(item.toObject());
                            auto type = JSONItem::ItemType::OBJECT;
                            pos_t block_size = sizeof(type) + sizeof(pos_t) + key.length() + 1 + _GetBinarySize(data);
                            container.add(std::addressof(block_size), sizeof(block_size));
                            container.add(std::addressof(type), sizeof(type));
                            container.add(key.data(), (key.length() + 1));
                            container.add(data, _GetBinarySize(data));
                            _DeleteBinary(data);
                        }
                        break;
                    default:
                        break;
                }
            };

            // Object Binary Structure: [binary_size][[block_size][type][key][data]]...[end_block]
            _Binary_Container container{};
            {
                pos_t binary_size = sizeof(pos_t);
                container.add(std::addressof(binary_size), sizeof(binary_size));
            }
            auto pairs = object.toPairs();
            for (const auto &pair : pairs) {
                addBlock(container, pair.key, pair.value);
            }
            container.add(std::addressof(npos), sizeof(npos));
            *reinterpret_cast<pos_t*>(container.data()) = container.size();
            return container.dump();
        }

        inline CU::JSONObject _BinaryToObject(const byte_t* binary)
        {
            static const auto addItem = [](JSONObject &object, const byte_t* binary, pos_t block_offset) -> pos_t {
                auto block_size = *reinterpret_cast<const pos_t*>(_Move_Ptr(binary, block_offset));
                if (block_size == npos) {
                    return npos;
                }
                auto type = *reinterpret_cast<const JSONItem::ItemType*>(_Move_Ptr(binary, (block_offset + sizeof(block_size))));
                auto key = reinterpret_cast<const char*>(_Move_Ptr(binary, (block_offset + sizeof(block_size) + sizeof(type))));
                auto key_size = std::strlen(key) + 1;
                auto data = _Move_Ptr(binary, (block_offset + sizeof(block_size) + sizeof(type) + key_size));
                switch (type) {
                    case JSONItem::ItemType::ITEM_NULL:
                        object.add(key, nullptr);
                        break;
                    case JSONItem::ItemType::BOOLEAN:
                        object.add(key, *reinterpret_cast<const bool*>(data));
                        break;
                    case JSONItem::ItemType::INTEGER:
                        object.add(key, *reinterpret_cast<const int*>(data));
                        break;
                    case JSONItem::ItemType::LONG:
                        object.add(key, *reinterpret_cast<const int64_t*>(data));
                        break;
                    case JSONItem::ItemType::DOUBLE:
                        object.add(key, *reinterpret_cast<const double*>(data));
                        break;
                    case JSONItem::ItemType::STRING:
                        object.add(key, reinterpret_cast<const char*>(data));
                        break;
                    case JSONItem::ItemType::ARRAY:
                        object.add(key, _BinaryToArray(reinterpret_cast<const byte_t*>(data)));
                        break;
                    case JSONItem::ItemType::OBJECT:
                        object.add(key, _BinaryToObject(reinterpret_cast<const byte_t*>(data)));
                        break;
                    default:
                        break;
                }
                return (block_offset + block_size);
            };

            JSONObject object{};
            pos_t size = _GetBinarySize(binary), offset = sizeof(pos_t);
            while (offset < size) {
                offset = addItem(object, binary, offset);
            }
            return object;
        }

        inline void SaveObject(const std::string &path, const JSONObject &object)
        {
            auto fp = std::fopen(path.c_str(), "wb");
            if (fp != nullptr) {
                auto binary = _ObjectToBinary(object);
                if (binary != nullptr) {
                    std::fwrite(binary, sizeof(byte_t), _GetBinarySize(binary), fp);
                    std::fflush(fp);
                    _DeleteBinary(binary);
                }
                std::fclose(fp);
            }
        }

        inline CU::JSONObject OpenObject(const std::string &path)
        {
            CU::JSONObject object{};
            auto fp = std::fopen(path.c_str(), "rb");
            if (fp != nullptr) {
                pos_t binary_size = 0;
                std::fread(std::addressof(binary_size), sizeof(binary_size), 1, fp);
                auto buffer = reinterpret_cast<byte_t*>(std::malloc(binary_size));
                if (buffer != nullptr) {
                    std::rewind(fp);
                    std::fread(buffer, sizeof(byte_t), binary_size, fp);
                    object = _BinaryToObject(buffer);
                    _DeleteBinary(buffer);
                }
                std::fclose(fp);
            }
            return object;
        }
    }
}

namespace std
{
    template <>
    struct hash<CU::JSONItem>
    {
        size_t operator()(const CU::JSONItem &val) const
        {
            return val.hash();
        }
    };

    template <>
    struct hash<CU::JSONArray>
    {
        size_t operator()(const CU::JSONArray &val) const
        {
            return val.hash();
        }
    };

    template <>
    struct hash<CU::JSONObject>
    {
        size_t operator()(const CU::JSONObject &val) const
        {
            return val.hash();
        }
    };
}

#endif // !defined(_CU_JSONOBJECT_)
