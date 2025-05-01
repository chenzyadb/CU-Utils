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
#include <limits>
#include <exception>
#include <algorithm>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <cstring>
#include <cinttypes>

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

    class _JSON_String
    {
        public:
            _JSON_String() noexcept : buffer_(), data_(nullptr), length_(0), capacity_(sizeof(buffer_)) { }

            _JSON_String(const _JSON_String &other) noexcept :
                buffer_(),
                data_(nullptr),
                length_(0),
                capacity_(sizeof(buffer_))
            {
                append(other);
            }

            _JSON_String(_JSON_String &&other) noexcept :
                buffer_(),
                data_(other.data_),
                length_(other.length_),
                capacity_(other.capacity_)
            {
                std::memcpy(buffer_, other.buffer_, sizeof(buffer_));
                other.data_ = nullptr;
            }

            _JSON_String(const char* src) noexcept :
                buffer_(),
                data_(nullptr),
                length_(0),
                capacity_(sizeof(buffer_))
            {
                append(src);
            }

            ~_JSON_String() noexcept
            {
                if (data_ != nullptr) {
                    std::free(data_);
                }
            }

            _JSON_String &operator=(const _JSON_String &other)
            {
                if (std::addressof(other) != this) {
                    shrink(0);
                    append(other.data());
                }
                return *this;
            }

            _JSON_String &operator=(_JSON_String &other) noexcept
            {
                if (std::addressof(other) != this) {
                    clear();
                    std::memcpy(buffer_, other.buffer_, sizeof(buffer_));
                    data_ = other.data_;
                    length_ = other.length_;
                    capacity_ = other.capacity_;
                    other.data_ = nullptr;
                }
                return *this;
            }

            void append(const _JSON_String &other) noexcept
            {
                auto new_len = length_ + other.length_;
                if (new_len >= capacity_) {
                    resize(capacity_ + new_len);
                }
                std::memcpy((data() + length_), other.data(), other.length_);
                *(data() + new_len) = '\0';
                length_ = new_len;
            }

            void append(const char* src) noexcept
            {
                auto src_len = std::strlen(src);
                auto new_len = length_ + src_len;
                if (new_len >= capacity_) {
                    resize(capacity_ + new_len);
                }
                std::memcpy((data() + length_), src, src_len);
                *(data() + new_len) = '\0';
                length_ = new_len;
            }

            void append(char ch) noexcept
            {
                if ((length_ + 1) >= capacity_) {
                    resize(capacity_ * 2);
                }
                *(data() + length_) = ch;
                *(data() + length_ + 1) = '\0';
                length_++;
            }

            void resize(size_t req_capacity) noexcept
            {
                if (req_capacity < capacity_ && length_ >= req_capacity) {
                    shrink(req_capacity - 1);
                }
                if (req_capacity > sizeof(buffer_)) {
                    auto new_block = reinterpret_cast<char*>(std::realloc(data_, req_capacity));
                    if (new_block != nullptr) {
                        if (data_ == nullptr) {
                            std::memcpy(new_block, buffer_, length_);
                        }
                        *(new_block + length_) = '\0';
                        data_ = new_block;
                        capacity_ = req_capacity;
                    }
                } else {
                    capacity_ = sizeof(buffer_);
                    if (data_ != nullptr && length_ > 0) {
                        std::memcpy(buffer_, data_, length_);
                        std::free(data_);
                    }
                    buffer_[length_] = '\0';
                }
            }

            void shrink(size_t req_length) noexcept
            {
                if (req_length < length_) {
                    *(data() + req_length) = '\0';
                    length_ = req_length;
                }
            }

            char* data() noexcept
            {
                if (data_ != nullptr) {
                    return data_;
                }
                return buffer_;
            }

            const char* data() const noexcept
            {
                if (data_ != nullptr) {
                    return data_;
                }
                return buffer_;
            }

            void clear() noexcept
            {
                if (data_ != nullptr) {
                    std::free(data_);
                    data_ = nullptr;
                }
                buffer_[0] = '\0';
                length_ = 0;
                capacity_ = sizeof(buffer_);
            }

            size_t length() const noexcept
            {
                return length_;
            }

        private:
            char buffer_[64];
            char* data_;
            size_t length_;
            size_t capacity_;
    };

    class JSONObject;
    class JSONArray;
    class JSONItem;

    inline _JSON_String _StringToJSONRaw(const std::string &str)
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

    class JSONItem
    {
        public:
            enum class ItemType : uint8_t {ITEM_NULL, BOOLEAN, INTEGER, LONG, DOUBLE, STRING, ARRAY, OBJECT};

            typedef decltype(nullptr) ItemNull;
            typedef std::variant<ItemNull, bool, int, int64_t, double, std::string, JSONArray*, JSONObject*> ItemValue;

            inline JSONItem();
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
            inline _JSON_String toRaw() const;

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
            inline JSONArray(std::vector<JSONItem> &&data) noexcept;
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
            inline _JSON_String toRaw() const;
            inline std::string toString() const;

            inline const std::vector<JSONItem> &data() const;

            inline JSONItem &front();
            inline JSONItem &back();
            inline const JSONItem &front() const;
            inline const JSONItem &back() const;
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
            inline _JSON_String toRaw() const;
            inline std::string toString() const;
            inline std::string toFormatedString() const;

            inline const std::unordered_map<std::string, JSONItem> &data() const;
            inline const std::vector<std::string> &order() const;

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

    namespace _JSON_Parse_Utils
    {
        inline void ThrowSyntaxExcept(std::string_view message, std::string_view jsonText = "", size_t beginPos = 0) 
        {
            static const auto countLine = [](std::string_view jsonText, size_t beginPos) -> size_t {
                if (beginPos > jsonText.size()) {
                    return 0;
                }

                size_t count = 0;
                for (size_t pos = 0; pos < beginPos; pos++) {
                    if (jsonText[pos] == '\n') {
                        count++;
                    }
                }
                return count;
            };

            if (jsonText.size() > 0) {
                char except_msg[4096]{};
                std::snprintf(except_msg, sizeof(except_msg), 
                    "Invalid syntax on line %zu: %s", (countLine(jsonText, beginPos) + 1), message.data());
                throw JSONExcept(except_msg);
            }
            throw JSONExcept(std::string("Invalid syntax: ") + message.data());
        }

        template <typename _Ty>
        struct Result 
        {
            _Ty resultVal;
            size_t endPos;

            Result() : resultVal(), endPos(-1) { }

            Result(const Result &other) : resultVal(other.resultVal), endPos(other.endPos) { }

            Result(Result &&other) noexcept : resultVal(std::move(other.resultVal)), endPos(other.endPos) { }
        };

        inline size_t IgnoreBlank(std::string_view jsonText, size_t beginPos) noexcept
        {
            static const auto isBlankChar = [](char ch) -> bool {
                switch (ch) {
                    case ' ':
                    case '\n':
                    case '\t':
                    case '\r':
                    case '\f':
                    case '\a':
                    case '\b':
                    case '\v':
                        return true;
                    default:
                        break;
                }
                return false;
            };

            for (auto pos = beginPos; pos < jsonText.size(); pos++) {
                if (!isBlankChar(jsonText[pos])) {
                    return pos;
                }
            }
            return std::string_view::npos;
        }

        inline Result<JSONArray> ParseJSONArray(std::string_view jsonText, size_t beginPos);

        inline Result<JSONObject> ParseJSONObject(std::string_view jsonText, size_t beginPos);

        inline Result<_JSON_String> ParseEscapeChar(std::string_view jsonText, size_t beginPos)
        {
            static const auto parseUnicode = [](std::string_view jsonText, size_t beginPos) -> _JSON_String
            {
                static constexpr int utf8bit_max = 0x7F;
                static constexpr int utf16bit_max = 0x7FF;
                static constexpr int utf24bit_max = 0xFFFF;
                static constexpr int utf32bit_max = 0x10FFFF;

                if (jsonText.size() >= (beginPos + 4)) {
                    int unicodeVal = std::strtol(jsonText.substr(beginPos, 4).data(), nullptr, 16);
                    if (unicodeVal <= utf8bit_max) {
                        char unicodeChar[2]{};
                        unicodeChar[0] = static_cast<char>(unicodeVal);
                        return unicodeChar;
                    } else if (unicodeVal <= utf16bit_max) {
                        char unicodeChar[3]{};
                        unicodeChar[0] = static_cast<char>(0xC0 | ((unicodeVal >> 6) & 0x1F));
                        unicodeChar[1] = static_cast<char>(0x80 | (unicodeVal & 0x3F));
                        return unicodeChar;
                    } else if (unicodeVal <= utf24bit_max) {
                        char unicodeChar[4]{};
                        unicodeChar[0] = static_cast<char>(0xE0 | ((unicodeVal >> 12) & 0x0F));
                        unicodeChar[1] = static_cast<char>(0x80 | ((unicodeVal >> 6) & 0x3F));
                        unicodeChar[2] = static_cast<char>(0x80 | (unicodeVal & 0x3F));
                        return unicodeChar;
                    } else if (unicodeVal <= utf32bit_max) {
                        char unicodeChar[5]{};
                        unicodeChar[0] = static_cast<char>(0xF0 | ((unicodeVal >> 18) & 0x07));
                        unicodeChar[1] = static_cast<char>(0x80 | ((unicodeVal >> 12) & 0x3F));
                        unicodeChar[2] = static_cast<char>(0x80 | ((unicodeVal >> 6) & 0x3F));
                        unicodeChar[3] = static_cast<char>(0x80 | (unicodeVal & 0x3F));
                        return unicodeChar;
                    }
                }

                ThrowSyntaxExcept("faild to parse unicode escape char", jsonText, beginPos);
                return {};
            };

            if (jsonText[beginPos] != '\\') {
                return {};
            }

            Result<_JSON_String> charResult{};
            auto pos = beginPos + 1;
            switch (jsonText[pos]) {
                case 'n':
                    charResult.resultVal.append('\n');
                    pos++;
                    break;
                case 'r':
                    charResult.resultVal.append('\r');
                    pos++;
                    break;
                case 't':
                    charResult.resultVal.append('\t');
                    pos++;
                    break;
                case 'b':
                    charResult.resultVal.append('\b');
                    pos++;
                    break;
                case 'f':
                    charResult.resultVal.append('\f');
                    pos++;
                    break;
                case 'a':
                    charResult.resultVal.append('\a');
                    pos++;
                    break;
                case 'v':
                    charResult.resultVal.append('\v');
                    pos++;
                    break;
                case '/':
                    charResult.resultVal.append('/');
                    pos++;
                    break;
                case '\\':
                    charResult.resultVal.append('\\');
                    pos++;
                    break;
                case '\"':
                    charResult.resultVal.append('\"');
                    pos++;
                    break;
                case '\'':
                    charResult.resultVal.append('\'');
                    pos++;
                    break;
                case 'u':
                    pos++;
                    charResult.resultVal.append(parseUnicode(jsonText, pos).data());
                    pos += 4;
                    break;
                default:
                    ThrowSyntaxExcept("unknown escape char", jsonText, pos);
                    break;
            }
            charResult.endPos = pos;
            return charResult;
        }

        inline Result<_JSON_String> ParseJSONString(std::string_view jsonText, size_t beginPos)
        {
            auto pos = IgnoreBlank(jsonText, beginPos);
            if (jsonText[pos] != '\"') {
                ThrowSyntaxExcept("JSON String must begin with \'\"\'", jsonText, beginPos);
            }
            pos++;

            Result<_JSON_String> stringResult{};
            while (pos < jsonText.size()) {
                if (jsonText[pos] == '\"') {
                    break;
                } else if (jsonText[pos] == '\\') {
                    auto charResult = ParseEscapeChar(jsonText, pos);
                    stringResult.resultVal.append(charResult.resultVal.data());
                    pos = charResult.endPos;
                } else {
                    stringResult.resultVal.append(jsonText[pos]);
                    pos++;
                }
            }
            if (pos >= jsonText.size()) {
                ThrowSyntaxExcept("JSON String must end with \'\"\'", jsonText, beginPos);
            }

            stringResult.endPos = pos + 1;
            return stringResult;
        }

        inline Result<JSONItem> ParseJSONItem(std::string_view jsonText, size_t beginPos)
        {
            static const auto compareText = 
                [](std::string_view text, size_t beginPos, std::string_view cmpText) -> bool 
            {
                if ((beginPos + cmpText.size()) >= text.size()) {
                    return false;
                }
                return (std::memcmp(&text[beginPos], cmpText.data(), cmpText.size()) == 0);
            };
            static const auto splitNumberText = [](std::string_view text, size_t beginPos) -> _JSON_String {
                _JSON_String numberText{};
                for (auto pos = beginPos; pos < text.size(); pos++) {
                    switch (text[pos]) {
                        case '-':
                        case '+':
                        case '.':
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
                        case 'e':
                        case 'E':
                            numberText.append(text[pos]);
                            break;
                        default:
                            return numberText;
                    }
                }
                return numberText;
            };

            Result<JSONItem> itemResult{};
            auto pos = IgnoreBlank(jsonText, beginPos);
            switch (jsonText[pos]) {
                case '{':
                    {
                        auto objectResult = ParseJSONObject(jsonText, pos);
                        itemResult.resultVal = std::move(objectResult.resultVal);
                        itemResult.endPos = objectResult.endPos;
                    }
                    break;
                case '[':
                    {
                        auto arrayResult = ParseJSONArray(jsonText, pos);
                        itemResult.resultVal = std::move(arrayResult.resultVal);
                        itemResult.endPos = arrayResult.endPos;
                    }
                    break;
                case '\"':
                    {
                        auto stringResult = ParseJSONString(jsonText, pos);
                        itemResult.resultVal = stringResult.resultVal.data();
                        itemResult.endPos = stringResult.endPos;
                    }
                    break;
                case '-':
                case '+':
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
                    {
                        auto numberText = splitNumberText(jsonText, pos);
                        if (std::string_view(numberText.data()).find('.') != std::string_view::npos) {
                            itemResult.resultVal = std::strtod(numberText.data(), nullptr);
                        } else {
                            auto number = std::strtoll(numberText.data(), nullptr, 10);
                            if (number >= std::numeric_limits<int>::min() && number <= std::numeric_limits<int>::max()) {
                                itemResult.resultVal = static_cast<int>(number);
                            } else {
                                itemResult.resultVal = number;
                            }
                        }
                        itemResult.endPos = pos + numberText.length();
                    }
                    break;
                case 'n':
                    if (compareText(jsonText, pos, "null")) {
                        itemResult.resultVal = JSONItem::ItemNull();
                        itemResult.endPos = pos + 4;
                    }
                    break;
                case 't':
                    if (compareText(jsonText, pos, "true")) {
                        itemResult.resultVal = true;
                        itemResult.endPos = pos + 4;
                    }
                    break;
                case 'f':
                    if (compareText(jsonText, pos, "false")) {
                        itemResult.resultVal = false;
                        itemResult.endPos = pos + 5;
                    }
                    break;
                default:
                    ThrowSyntaxExcept("unknown JSON Item", jsonText, pos);
                    break;
            }
            return itemResult;
        }

        inline Result<JSONArray> ParseJSONArray(std::string_view jsonText, size_t beginPos)
        {
            auto pos = IgnoreBlank(jsonText, beginPos);
            if (jsonText[pos] != '[') {
                ThrowSyntaxExcept("JSON Array must begin with \'[\'", jsonText, beginPos);
            }

            pos = IgnoreBlank(jsonText, (pos + 1));
            if (jsonText[pos] == ']') {
                Result<JSONArray> arrayResult{};
                arrayResult.endPos = pos + 1;
                return arrayResult;
            }

            std::vector<JSONItem> array{};
            while (pos < jsonText.size()) {
                auto itemResult = ParseJSONItem(jsonText, IgnoreBlank(jsonText, pos));
                array.emplace_back(std::move(itemResult.resultVal));

                pos = IgnoreBlank(jsonText, itemResult.endPos);
                if (jsonText[pos] == ']') {
                    break;
                } else if (jsonText[pos] == ',') {
                    pos++;
                } else {
                    ThrowSyntaxExcept("JSON Array elements must be separated by \',\'", jsonText, pos);
                }
            }
            if (pos >= jsonText.size()) {
                ThrowSyntaxExcept("JSON Array must end with \']\'", jsonText, beginPos);
            }

            Result<JSONArray> arrayResult{};
            arrayResult.resultVal = std::move(array);
            arrayResult.endPos = pos + 1;
            return arrayResult;
        }

        inline Result<JSONObject> ParseJSONObject(std::string_view jsonText, size_t beginPos)
        {
            auto pos = IgnoreBlank(jsonText, beginPos);
            if (jsonText[pos] != '{') {
                ThrowSyntaxExcept("JSON Object must begin with \'{\'", jsonText, pos);
            }

            pos = IgnoreBlank(jsonText, (pos + 1));
            if (jsonText[pos] == '}') {
                Result<JSONObject> objectResult{};
                objectResult.endPos = pos + 1;
                return objectResult;
            }

            std::unordered_map<std::string, JSONItem> data{};
            std::vector<std::string> order{};
            while (pos < jsonText.size()) {
                auto stringResult = ParseJSONString(jsonText, IgnoreBlank(jsonText, pos));

                pos = IgnoreBlank(jsonText, stringResult.endPos);
                if (jsonText[pos] != ':') {
                    ThrowSyntaxExcept("key and value must be separated by \':\'", jsonText, pos);
                }

                auto itemResult = ParseJSONItem(jsonText, IgnoreBlank(jsonText, (pos + 1)));
                data.emplace(stringResult.resultVal.data(), std::move(itemResult.resultVal));
                order.emplace_back(stringResult.resultVal.data());

                pos = IgnoreBlank(jsonText, itemResult.endPos);
                if (jsonText[pos] == '}') {
                    break;
                } else if (jsonText[pos] == ',') {
                    pos++;
                } else {
                    ThrowSyntaxExcept("JSON Object elements must be separated by \',\'", jsonText, pos);
                }
            }
            if (pos >= jsonText.size()) {
                ThrowSyntaxExcept("JSON Object must end with \'}\'", jsonText, beginPos);
            }

            Result<JSONObject> objectResult{};
            objectResult.resultVal = JSONObject(std::move(data), std::move(order));
            objectResult.endPos = pos + 1;
            return objectResult;
        }
    }

    inline JSONItem::JSONItem() :
        type_(ItemType::ITEM_NULL),
        value_(ItemNull())
    { }

    inline JSONItem::JSONItem(ItemNull _null) :
        type_(ItemType::ITEM_NULL),
        value_(ItemNull())
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

    inline _JSON_String JSONItem::toRaw() const
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
                {
                    char buffer[32]{};
                    std::snprintf(buffer, sizeof(buffer), "%d", std::get<int>(value_));
                    return buffer;
                }
            case ItemType::LONG:
                {
                    char buffer[32]{};
                    std::snprintf(buffer, sizeof(buffer), ("%" PRId64) , std::get<int64_t>(value_));
                    return buffer;
                }
            case ItemType::DOUBLE:
                {
                    char buffer[32]{};
                    std::snprintf(buffer, sizeof(buffer), "%.8f", std::get<double>(value_));
                    return buffer;
                }
            case ItemType::STRING:
                return _StringToJSONRaw(std::get<std::string>(value_));
            case ItemType::ARRAY:
                return std::get<JSONArray*>(value_)->toRaw();
            case ItemType::OBJECT:
                return std::get<JSONObject*>(value_)->toRaw();
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
        auto arrayResult = _JSON_Parse_Utils::ParseJSONArray(jsonText, 0);
        auto array = std::move(arrayResult.resultVal);
        data_ = std::move(array.data_);
    }

    inline JSONArray::JSONArray(const std::vector<JSONItem> &data) : data_(data) { }

    inline JSONArray::JSONArray(std::vector<JSONItem> &&data) noexcept : data_(data) { }

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
            for (auto iter = other_data.begin(); iter < other_data.end(); ++iter) {
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
            for (auto iter = other_data.begin(); iter < other_data.end(); ++iter) {
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

    inline _JSON_String JSONArray::toRaw() const
    {
        if (data_.begin() == data_.end()) {
            return "[]";
        } else if ((data_.begin() + 1) == data_.end()) {
            _JSON_String jsonText("[");
            jsonText.append(data_.front().toRaw().data());
            jsonText.append(']');
            return jsonText;
        }
        _JSON_String jsonText("[");
        for (auto iter = data_.begin(); iter < (data_.end() - 1); ++iter) {
            jsonText.append((iter->toRaw()).data());
            jsonText.append(',');
        }
        jsonText.append(data_.back().toRaw().data());
        jsonText.append(']');
        return jsonText;
    }

    inline std::string JSONArray::toString() const
    {
        return toRaw().data();
    }

    inline const std::vector<JSONItem> &JSONArray::data() const
    {
        return data_;
    }

    inline JSONItem &JSONArray::front()
    {
        return data_.front();
    }

    inline JSONItem &JSONArray::back()
    {
        return data_.back();
    }

    inline const JSONItem &JSONArray::front() const
    {
        return data_.front();
    }

    inline const JSONItem &JSONArray::back() const
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
        auto objectResult = _JSON_Parse_Utils::ParseJSONObject(jsonText, 0);
        auto object = std::move(objectResult.resultVal);
        data_ = std::move(object.data_);
        order_ = std::move(object.order_);
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
        for (auto iter = data_.begin(); iter != data_.end(); ++iter) {
            hashVal ^= (iter->second).hash() + 2654435769 + (hashVal << 6) + (hashVal >> 2);
        }
        return hashVal;
    }

    inline bool JSONObject::empty() const
    {
        return (data_.begin() == data_.end());
    }

    inline _JSON_String JSONObject::toRaw() const
    {
        if (order_.size() == 1) {
            _JSON_String jsonText("{");
            jsonText.append(_StringToJSONRaw(order_.front()));
            jsonText.append(':');
            jsonText.append(data_.at(order_.front()).toRaw().data());
            jsonText.append('}');
            return jsonText;
        } else if (order_.size() > 1) {
            _JSON_String jsonText("{");
            for (auto iter = order_.begin(); iter < (order_.end() - 1); ++iter) {
                const auto &key = *iter;
                jsonText.append(_StringToJSONRaw(key));
                jsonText.append(':');
                jsonText.append(data_.at(key).toRaw().data());
                jsonText.append(',');
            }
            jsonText.append(_StringToJSONRaw(order_.back()));
            jsonText.append(':');
            jsonText.append(data_.at(order_.back()).toRaw().data());
            jsonText.append('}');
            return jsonText;
        }
        return "{}";
    }

    inline std::string JSONObject::toString() const
    {
        return toRaw().data();
    }

    inline std::string JSONObject::toFormatedString() const
    {
        if (order_.size() == 1) {
            _JSON_String jsonText("{\n");
            jsonText.append(_StringToJSONRaw(order_.front()));
            jsonText.append(": ");
            jsonText.append(data_.at(order_.front()).toRaw().data());
            jsonText.append("\n }");
            return jsonText.data();
        } else if (order_.size() > 1) {
            _JSON_String jsonText("{\n");
            for (auto iter = order_.begin(); iter < (order_.end() - 1); ++iter) {
                const auto &key = *iter;
                jsonText.append("  ");
                jsonText.append(_StringToJSONRaw(key));
                jsonText.append(": ");
                jsonText.append(data_.at(key).toRaw().data());
                jsonText.append(",\n");
            }
            jsonText.append("  ");
            jsonText.append(_StringToJSONRaw(order_.back()));
            jsonText.append(": ");
            jsonText.append(data_.at(order_.back()).toRaw().data());
            jsonText.append("\n}");
            return jsonText.data();
        }
        return "{ }";
    }

    inline const std::unordered_map<std::string, JSONItem> &JSONObject::data() const
    {
        return data_;
    }

    inline const std::vector<std::string> &JSONObject::order() const
    {
        return order_;
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
            static const auto addItem = 
                [](std::vector<JSONItem> &arrayData, const byte_t* binary, pos_t block_offset) -> pos_t 
            {
                auto block_size = *reinterpret_cast<const pos_t*>(_Move_Ptr(binary, block_offset));
                if (block_size == npos) {
                    return npos;
                }
                auto type = *reinterpret_cast<const JSONItem::ItemType*>(_Move_Ptr(binary, (block_offset + sizeof(block_size))));
                auto data = _Move_Ptr(binary, (block_offset + sizeof(block_size) + sizeof(type)));
                switch (type) {
                    case JSONItem::ItemType::ITEM_NULL:
                        arrayData.emplace_back(nullptr);
                        break;
                    case JSONItem::ItemType::BOOLEAN:
                        arrayData.emplace_back(*reinterpret_cast<const bool*>(data));
                        break;
                    case JSONItem::ItemType::INTEGER:
                        arrayData.emplace_back(*reinterpret_cast<const int*>(data));
                        break;
                    case JSONItem::ItemType::LONG:
                        arrayData.emplace_back(*reinterpret_cast<const int64_t*>(data));
                        break;
                    case JSONItem::ItemType::DOUBLE:
                        arrayData.emplace_back(*reinterpret_cast<const double*>(data));
                        break;
                    case JSONItem::ItemType::STRING:
                        arrayData.emplace_back(reinterpret_cast<const char*>(data));
                        break;
                    case JSONItem::ItemType::ARRAY:
                        arrayData.emplace_back(_BinaryToArray(reinterpret_cast<const byte_t*>(data)));
                        break;
                    case JSONItem::ItemType::OBJECT:
                        arrayData.emplace_back(_BinaryToObject(reinterpret_cast<const byte_t*>(data)));
                        break;
                    default:
                        break;
                }
                return (block_offset + block_size);
            };

            std::vector<JSONItem> arrayData{};
            pos_t size = _GetBinarySize(binary), offset = sizeof(pos_t);
            while (offset < size) {
                offset = addItem(arrayData, binary, offset);
            }
            return JSONArray(std::move(arrayData));
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
            const auto &objectData = object.data();
            const auto &objectOrder = object.order();
            for (const auto &key : objectOrder) {
                addBlock(container, key, objectData.at(key));
            }
            container.add(std::addressof(npos), sizeof(npos));
            *reinterpret_cast<pos_t*>(container.data()) = container.size();
            return container.dump();
        }

        inline CU::JSONObject _BinaryToObject(const byte_t* binary)
        {
            static const auto addItem = [](
                std::unordered_map<std::string, JSONItem> &objectData, 
                std::vector<std::string> &objectOrder,
                const byte_t* binary, 
                pos_t block_offset
            ) -> pos_t {
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
                        objectData.emplace(key, nullptr);
                        objectOrder.emplace_back(key);
                        break;
                    case JSONItem::ItemType::BOOLEAN:
                        objectData.emplace(key, *reinterpret_cast<const bool*>(data));
                        objectOrder.emplace_back(key);
                        break;
                    case JSONItem::ItemType::INTEGER:
                        objectData.emplace(key, *reinterpret_cast<const int*>(data));
                        objectOrder.emplace_back(key);
                        break;
                    case JSONItem::ItemType::LONG:
                        objectData.emplace(key, *reinterpret_cast<const int64_t*>(data));
                        objectOrder.emplace_back(key);
                        break;
                    case JSONItem::ItemType::DOUBLE:
                        objectData.emplace(key, *reinterpret_cast<const double*>(data));
                        objectOrder.emplace_back(key);
                        break;
                    case JSONItem::ItemType::STRING:
                        objectData.emplace(key, reinterpret_cast<const char*>(data));
                        objectOrder.emplace_back(key);
                        break;
                    case JSONItem::ItemType::ARRAY:
                        objectData.emplace(key, _BinaryToArray(reinterpret_cast<const byte_t*>(data)));
                        objectOrder.emplace_back(key);
                        break;
                    case JSONItem::ItemType::OBJECT:
                        objectData.emplace(key, _BinaryToObject(reinterpret_cast<const byte_t*>(data)));
                        objectOrder.emplace_back(key);
                        break;
                    default:
                        break;
                }
                return (block_offset + block_size);
            };

            std::unordered_map<std::string, JSONItem> objectData{};
            std::vector<std::string> objectOrder{};
            pos_t size = _GetBinarySize(binary), offset = sizeof(pos_t);
            while (offset < size) {
                offset = addItem(objectData, objectOrder, binary, offset);
            }
            return JSONObject(std::move(objectData), std::move(objectOrder));
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
