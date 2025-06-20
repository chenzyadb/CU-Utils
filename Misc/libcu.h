// cuprum cross-platform library by chenzyadb@github.com
// Based on C++17 STL (LLVM)

#ifndef __LIB_CU__
#define __LIB_CU__ 1

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <limits>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cstdarg>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <cinttypes>
#include <cwchar>

#define CU_UNUSED(val) (void)(val)
#define CU_WCHAR(val) L##val

#if defined(__GNUC__)
#define CU_INLINE __attribute__((always_inline)) inline
#define CU_LIKELY(val) (__builtin_expect(!!(val), 1))
#define CU_UNLIKELY(val) (__builtin_expect(!!(val), 0))
#elif defined(_MSC_VER)
#pragma warning(disable : 4996)
#define CU_INLINE __forceinline
#define CU_LIKELY(val) (!!(val))
#define CU_UNLIKELY(val) (!!(val))
#else
#define CU_INLINE inline
#define CU_LIKELY(val) (!!(val))
#define CU_UNLIKELY(val) (!!(val))
#endif


namespace CU
{
    CU_INLINE void WriteFile(const std::string &path, const std::string &content)
    {
        auto fp = std::fopen(path.data(), "w");
        if (fp != nullptr) {
            std::fputs(content.data(), fp);
            std::fflush(fp);
            std::fclose(fp);
        }
    }

    CU_INLINE std::string ReadFile(const std::string &path)
    {
        auto fp = std::fopen(path.data(), "r");
        if (fp != nullptr) {
            std::string content{};
            char buffer[4096]{};
            while (std::fread(buffer, 1, (sizeof(buffer) - 1), fp) > 0) {
                content += buffer;
                std::memset(buffer, 0, sizeof(buffer));
            }
            std::fclose(fp);
            return content;
        }
        return {};
    }

    CU_INLINE bool CStrEquals(const char* str1, const char* str2) noexcept
    {
        auto str_len = std::strlen(str1);
        if (str_len != std::strlen(str2)) {
            return false;
        }
        return (std::memcmp(str1, str2, str_len) == 0);
    }

    CU_INLINE std::string WcsToStr(const std::wstring &wc_str)
    {
        std::string str{};
        auto str_len = (wc_str.size() + 1) * sizeof(wchar_t);
        auto str_buf = new char[str_len];
        std::memset(str_buf, 0, str_len);
        mbstate_t mbstate{};
        auto wcs_data = wc_str.data();
        if (std::wcsrtombs(str_buf, &wcs_data, str_len, &mbstate) != static_cast<size_t>(-1)) {
            str = str_buf;
        }
        delete[] str_buf;
        return str;
    }

    CU_INLINE std::wstring StrToWcs(const std::string &str)
    {
        std::wstring wc_str{};
        auto wcs_len = str.size() + 1;
        auto wcs_buf = new wchar_t[wcs_len];
        std::memset(wcs_buf, 0, wcs_len * sizeof(wchar_t));
        mbstate_t mbstate{};
        auto str_data = str.data();
        if (std::mbsrtowcs(wcs_buf, &str_data, wcs_len, &mbstate) != static_cast<size_t>(-1)) {
            wc_str = wcs_buf;
        }
        delete[] wcs_buf;
        return wc_str;
    }

    template <typename _Char_Ty>
    inline std::vector<std::basic_string<_Char_Ty>>
    StrSplit(const std::basic_string<_Char_Ty> &str, const std::basic_string<_Char_Ty> &delimiter)
    {
        if (CU_UNLIKELY(delimiter.size() == 0 || str.size() <= delimiter.size())) {
            return {};
        }
        std::vector<std::basic_string<_Char_Ty>> splittedStrings{};
        size_t start_pos = 0;
        size_t pos = str.find(delimiter);
        while (pos != std::basic_string<_Char_Ty>::npos) {
            if (start_pos < pos) {
                splittedStrings.emplace_back(str.substr(start_pos, (pos - start_pos)));
            }
            start_pos = pos + delimiter.size();
            pos = str.find(delimiter, start_pos);
        }
        if (start_pos < str.size()) {
            splittedStrings.emplace_back(str.substr(start_pos));
        }
        return splittedStrings;
    }

    template <typename _Char_Ty>
    inline std::vector<std::basic_string<_Char_Ty>>
    StrSplit(const std::basic_string<_Char_Ty> &str, const _Char_Ty* delimiter)
    {
        size_t delimiter_size = 0;
        for (size_t pos = 0; pos < std::numeric_limits<size_t>::max(); pos++) {
            if (*(delimiter + pos) == static_cast<_Char_Ty>(0)) {
                delimiter_size = pos;
                break;
            }
        }
        if (CU_UNLIKELY(delimiter_size == 0 || str.size() <= delimiter_size)) {
            return {};
        }
        std::vector<std::basic_string<_Char_Ty>> splittedStrings{};
        size_t start_pos = 0;
        size_t pos = str.find(delimiter);
        while (pos != std::basic_string<_Char_Ty>::npos) {
            if (start_pos < pos) {
                splittedStrings.emplace_back(str.substr(start_pos, (pos - start_pos)));
            }
            start_pos = pos + delimiter_size;
            pos = str.find(delimiter, start_pos);
        }
        if (start_pos < str.size()) {
            splittedStrings.emplace_back(str.substr(start_pos));
        }
        return splittedStrings;
    }

    template <typename _Char_Ty>
    inline std::vector<std::basic_string<_Char_Ty>>
    StrSplit(const std::basic_string<_Char_Ty> &str, _Char_Ty delimiter) 
    {
        if (CU_UNLIKELY(str.size() <= 1)) {
            return {};
        }
        std::vector<std::basic_string<_Char_Ty>> splittedStrings{};
        size_t start_pos = 0;
        for (size_t pos = 0; pos < str.size(); pos++) {
            if (str[pos] == delimiter || pos == (str.size() - 1)) {
                if (start_pos < pos) {
                    splittedStrings.emplace_back(str.substr(start_pos, (pos - start_pos)));
                }
                start_pos = pos + 1;
            }
        }
        return splittedStrings;
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty>
    StrSplitAt(const std::basic_string<_Char_Ty> &str, const std::basic_string<_Char_Ty> &delimiter, int targetCount) 
    {
        if (CU_UNLIKELY(delimiter.size() == 0 || str.size() <= delimiter.size())) {
            return {};
        }
        int count = 0;
        size_t start_pos = 0;
        size_t pos = str.find(delimiter);
        while (pos != std::basic_string<_Char_Ty>::npos) {
            if (start_pos < pos) {
                if (count == targetCount) {
                    return str.substr(start_pos, (pos - start_pos));
                }
                count++;
            }
            start_pos = pos + delimiter.size();
            pos = str.find(delimiter, start_pos);
        }
        if (start_pos < str.size() && count == targetCount) {
            return str.substr(start_pos);
        }
        return {};
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty>
    StrSplitAt(const std::basic_string<_Char_Ty> &str, const _Char_Ty* delimiter, int targetCount) 
    {
        size_t delimiter_size = 0;
        for (size_t pos = 0; pos < std::numeric_limits<size_t>::max(); pos++) {
            if (*(delimiter + pos) == static_cast<_Char_Ty>(0)) {
                delimiter_size = pos;
                break;
            }
        }
        if (CU_UNLIKELY(delimiter_size == 0 || str.size() <= delimiter_size)) {
            return {};
        }
        int count = 0;
        size_t start_pos = 0;
        size_t pos = str.find(delimiter);
        while (pos != std::basic_string<_Char_Ty>::npos) {
            if (start_pos < pos) {
                if (count == targetCount) {
                    return str.substr(start_pos, (pos - start_pos));
                }
                count++;
            }
            start_pos = pos + delimiter_size;
            pos = str.find(delimiter, start_pos);
        }
        if (start_pos < str.size() && count == targetCount) {
            return str.substr(start_pos);
        }
        return {};
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty> 
    StrSplitAt(const std::basic_string<_Char_Ty> &str, _Char_Ty delimiter, int targetCount) 
    {
        if (CU_UNLIKELY(str.size() <= 1)) {
            return {};
        }
        int count = 0;
        size_t start_pos = 0;
        for (size_t pos = 0; pos < str.size(); pos++) {
            if (str[pos] == delimiter || pos == (str.size() - 1)) {
                if (start_pos < pos) {
                    if (count == targetCount) {
                        return str.substr(start_pos, (pos - start_pos));
                    }
                    count++;
                }
                start_pos = pos + 1;
            }
        }
        return {};
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty>
    SubPrevStr(const std::basic_string<_Char_Ty> &str, const std::basic_string<_Char_Ty> &delimiter)
    {
        if (CU_UNLIKELY(delimiter.size() == 0 || str.size() <= delimiter.size())) {
            return str;
        }
        return str.substr(0, str.find(delimiter));
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty> 
    SubPrevStr(const std::basic_string<_Char_Ty> &str, const _Char_Ty* delimiter)
    {
        size_t delimiter_size = 0;
        for (size_t pos = 0; pos < std::numeric_limits<size_t>::max(); pos++) {
            if (*(delimiter + pos) == static_cast<_Char_Ty>(0)) {
                delimiter_size = pos;
                break;
            }
        }
        if (CU_UNLIKELY(delimiter_size == 0 || str.size() < delimiter_size)) {
            return str;
        }
        return str.substr(0, str.find(delimiter));
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty> 
    SubPrevStr(const std::basic_string<_Char_Ty> &str, _Char_Ty delimiter)
    {
        if (CU_UNLIKELY(str.size() <= 1)) {
            return str;
        }
        for (size_t pos = 0; pos < str.size(); pos++) {
            if (str[pos] == delimiter) {
                return str.substr(0, pos);
            }
        }
        return str;
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty>
    SubRePrevStr(const std::basic_string<_Char_Ty> &str, const std::basic_string<_Char_Ty> &delimiter)
    {
        if (CU_UNLIKELY(delimiter.size() == 0 || str.size() <= delimiter.size())) {
            return str;
        }
        return str.substr(0, str.rfind(delimiter));
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty> 
    SubRePrevStr(const std::basic_string<_Char_Ty> &str, const _Char_Ty* delimiter)
    {
        size_t delimiter_size = 0;
        for (size_t pos = 0; pos < std::numeric_limits<size_t>::max(); pos++) {
            if (*(delimiter + pos) == static_cast<_Char_Ty>(0)) {
                delimiter_size = pos;
                break;
            }
        }
        if (CU_UNLIKELY(delimiter_size == 0 || str.size() < delimiter_size)) {
            return str;
        }
        return str.substr(0, str.rfind(delimiter));
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty> 
    SubRePrevStr(const std::basic_string<_Char_Ty> &str, _Char_Ty delimiter)
    {
        if (CU_UNLIKELY(str.size() <= 1)) {
            return str;
        }
        for (size_t pos = (str.size() - 1); pos >= 0; pos--) {
            if (str[pos] == delimiter) {
                return str.substr(0, pos);
            }
        }
        return str;
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty>
    SubPostStr(const std::basic_string<_Char_Ty> &str, const std::basic_string<_Char_Ty> &delimiter)
    {
        if (CU_UNLIKELY(delimiter.size() == 0 || str.size() <= delimiter.size())) {
            return {};
        }
        auto sub_pos = str.find(delimiter);
        if (sub_pos != std::basic_string<_Char_Ty>::npos) {
            return str.substr(sub_pos + delimiter.size());
        }
        return {};
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty>
    SubPostStr(const std::basic_string<_Char_Ty> &str, const _Char_Ty* delimiter)
    {
        size_t delimiter_size = 0;
        for (size_t pos = 0; pos < std::numeric_limits<size_t>::max(); pos++) {
            if (*(delimiter + pos) == static_cast<_Char_Ty>(0)) {
                delimiter_size = pos;
                break;
            }
        }
        if (CU_UNLIKELY(delimiter_size == 0 || str.size() <= delimiter_size)) {
            return {};
        }
        auto sub_pos = str.find(delimiter);
        if (sub_pos != std::basic_string<_Char_Ty>::npos) {
            return str.substr(sub_pos + delimiter_size);
        }
        return {};
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty> 
    SubPostStr(const std::basic_string<_Char_Ty> &str, _Char_Ty delimiter)
    {
        if (CU_UNLIKELY(str.size() <= 1)) {
            return {};
        }
        for (size_t pos = 0; pos < (str.size() - 1); pos++) {
            if (str[pos] == delimiter) {
                return str.substr(pos + 1);
            }
        }
        return {};
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty>
    SubRePostStr(const std::basic_string<_Char_Ty> &str, const std::basic_string<_Char_Ty> &delimiter)
    {
        if (CU_UNLIKELY(delimiter.size() == 0 || str.size() <= delimiter.size())) {
            return {};
        }
        auto sub_pos = str.rfind(delimiter);
        if (sub_pos != std::basic_string<_Char_Ty>::npos) {
            return str.substr(sub_pos + delimiter.size());
        }
        return {};
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty>
    SubRePostStr(const std::basic_string<_Char_Ty> &str, const _Char_Ty* delimiter)
    {
        size_t delimiter_size = 0;
        for (size_t pos = 0; pos < std::numeric_limits<size_t>::max(); pos++) {
            if (*(delimiter + pos) == static_cast<_Char_Ty>(0)) {
                delimiter_size = pos;
                break;
            }
        }
        if (CU_UNLIKELY(delimiter_size == 0 || str.size() <= delimiter_size)) {
            return {};
        }
        auto sub_pos = str.rfind(delimiter);
        if (sub_pos != std::basic_string<_Char_Ty>::npos) {
            return str.substr(sub_pos + delimiter_size);
        }
        return {};
    }

    template <typename _Char_Ty>
    inline std::basic_string<_Char_Ty> 
    SubRePostStr(const std::basic_string<_Char_Ty> &str, _Char_Ty delimiter)
    {
        if (CU_UNLIKELY(str.size() <= 1)) {
            return {};
        }
        for (size_t pos = (str.size() - 2); pos >= 0; pos--) {
            if (str[pos] == delimiter) {
                return str.substr(pos + 1);
            }
        }
        return {};
    }

    template <typename _Char_Ty>
    inline bool 
    StrContains(const std::basic_string<_Char_Ty> &str, const std::basic_string<_Char_Ty> &key) noexcept
    {
        return (str.find(key) != std::string::npos);
    }

    template <typename _Char_Ty>
    inline bool 
    StrContains(const std::basic_string<_Char_Ty> &str, const _Char_Ty* key) noexcept
    {
        return (str.find(key) != std::string::npos);
    }

    template <typename _Char_Ty>
    inline bool 
    StrStartsWith(const std::basic_string<_Char_Ty> &str, const std::basic_string<_Char_Ty> &key) noexcept
    {
        if (CU_UNLIKELY(key.size() == 0 || str.size() < key.size())) {
            return false;
        }
        return (std::memcmp(str.data(), key.data(), (key.size() * sizeof(_Char_Ty))) == 0);
    }

    template <typename _Char_Ty>
    inline bool 
    StrStartsWith(const std::basic_string<_Char_Ty> &str, const _Char_Ty* key) noexcept
    {
        size_t key_size = 0;
        for (size_t pos = 0; pos < std::numeric_limits<size_t>::max(); pos++) {
            if (*(key + pos) == static_cast<_Char_Ty>(0)) {
                key_size = pos;
                break;
            }
        }
        if (CU_UNLIKELY(key_size == 0 || str.size() < key_size)) {
            return false;
        }
        return (std::memcmp(str.data(), key, (key_size * sizeof(_Char_Ty))) == 0);
    }

    template <typename _Char_Ty>
    inline bool 
    StrEndsWith(const std::basic_string<_Char_Ty> &str, const std::basic_string<_Char_Ty> &key) noexcept
    {
        if (CU_UNLIKELY(key.size() == 0 || str.size() < key.size())) {
            return false;
        }
        return (std::memcmp((str.data() + (str.size() - key.size())), 
            key.data(), (key.size() * sizeof(_Char_Ty))) == 0);
    }

    template <typename _Char_Ty>
    inline bool StrEndsWith(const std::basic_string<_Char_Ty> &str, const _Char_Ty* key) noexcept
    {
        size_t key_size = 0;
        for (size_t pos = 0; pos < std::numeric_limits<size_t>::max(); pos++) {
            if (*(key + pos) == static_cast<_Char_Ty>(0)) {
                key_size = pos;
                break;
            }
        }
        if (CU_UNLIKELY(key_size == 0 || str.size() < key_size)) {
            return false;
        }
        return (std::memcmp((str.data() + (str.size() - key_size)), key, (key_size * sizeof(_Char_Ty))) == 0);
    }

    CU_INLINE int StrToInt(const std::string &str) noexcept
    {
        long integer = std::strtol(str.data(), nullptr, 10);
        if (integer > std::numeric_limits<int>::max()) {
            return std::numeric_limits<int>::max();
        }
        if (integer < std::numeric_limits<int>::min()) {
            return std::numeric_limits<int>::min();
        }
        return static_cast<int>(integer);
    }

    CU_INLINE int StrToInt(const std::wstring &str) noexcept
    {
        long integer = std::wcstol(str.data(), nullptr, 10);
        if (integer > std::numeric_limits<int>::max()) {
            return std::numeric_limits<int>::max();
        }
        if (integer < std::numeric_limits<int>::min()) {
            return std::numeric_limits<int>::min();
        }
        return static_cast<int>(integer);
    }

    CU_INLINE int64_t StrToLong(const std::string &str) noexcept
    {
        return std::strtoll(str.data(), nullptr, 10);
    }

    CU_INLINE int64_t StrToLong(const std::wstring &str) noexcept
    {
        return std::wcstoll(str.data(), nullptr, 10);
    }

    CU_INLINE uint64_t StrToULong(const std::string &str) noexcept
    {
        return std::strtoull(str.data(), nullptr, 10);
    }

    CU_INLINE uint64_t StrToULong(const std::wstring &str) noexcept
    {
        return std::wcstoull(str.data(), nullptr, 10);
    }

    CU_INLINE double StrToDouble(const std::string &str) noexcept
    {
        return std::strtod(str.data(), nullptr);
    }

    CU_INLINE double StrToDouble(const std::wstring &str) noexcept
    {
        return std::wcstod(str.data(), nullptr);
    }

    CU_INLINE int64_t HexToInt(const std::string &str) noexcept
    {
        return std::strtoll(str.data(), nullptr, 16);
    }

    CU_INLINE int64_t HexToInt(const std::wstring &str) noexcept
    {
        return std::wcstoll(str.data(), nullptr, 16);
    }

    CU_INLINE std::string TrimStr(const std::string &str) 
    {
        std::string trimedStr{};
        for (size_t pos = 0; pos < str.size(); pos++) {
            char ch = str[pos];
            if (ch != ' ' && ch != '\n' && ch != '\t' && ch != '\r' && ch != '\b' && ch != '\v' && ch != '\f') {
                trimedStr += ch;
            }
        }
        return trimedStr;
    }

    CU_INLINE std::wstring TrimStr(const std::wstring &str) 
    {
        std::wstring trimedStr{};
        for (size_t pos = 0; pos < str.size(); pos++) {
            auto ch = str[pos];
            if (ch != ' ' && ch != '\n' && ch != '\t' && ch != '\r' && ch != '\b' && ch != '\v' && ch != '\f') {
                trimedStr += ch;
            }
        }
        return trimedStr;
    }

    template <typename _Ty>
    inline size_t Hash(const _Ty &val)
    {
        std::hash<_Ty> hashVal{};
        return hashVal(val);
    }

    template <typename _Ty>
    inline bool Compare(const _Ty &val0, const _Ty &val1) noexcept
    {
        const auto val0_addr = std::addressof(val0);
        const auto val1_addr = std::addressof(val1);
        if (CU_UNLIKELY(val0_addr == val1_addr)) {
            return true;
        }
        return (std::memcmp(val0_addr, val1_addr, sizeof(_Ty)) == 0);
    }

    template <typename _Ty>
    inline void Copy(_Ty &dst, const _Ty &src) noexcept
    {
        const auto src_addr = std::addressof(src);
        auto dst_addr = std::addressof(dst);
        if (CU_LIKELY(src_addr != dst_addr)) {
            std::memcpy(dst_addr, src_addr, sizeof(_Ty));
        }
    }

    template <typename _Num_Ty>
    inline void Max_Impl(_Num_Ty &max, _Num_Ty num) noexcept
    {
        if (num > max) {
            max = num;
        }
    }

    template <typename _Num_Ty, typename ..._Nums>
    inline void Max_Impl(_Num_Ty &max, _Num_Ty num, _Nums ...nums) noexcept
    {
        if (num > max) {
            max = num;
        }
        Max_Impl(max, nums...);
    }

    template <typename _Num_Ty, typename ..._Nums>
    inline _Num_Ty Max(_Num_Ty num, _Nums ...nums) noexcept
    {
        auto max = num;
        Max_Impl(max, nums...);
        return max;
    }

    template <typename _Num_Ty>
    inline void Min_Impl(_Num_Ty &min, _Num_Ty num) noexcept
    {
        if (num < min) {
            min = num;
        }
    }

    template <typename _Num_Ty, typename ..._Nums>
    inline void Min_Impl(_Num_Ty &min, _Num_Ty num, _Nums ...nums) noexcept
    {
        if (num < min) {
            min = num;
        }
        Min_Impl(min, nums...);
    }

    template <typename _Num_Ty, typename ..._Nums>
    inline _Num_Ty Min(_Num_Ty num, _Nums ...nums) noexcept
    {
        auto min = num;
        Min_Impl(min, nums...);
        return min;
    }

    template <typename _Ty>
    inline int64_t Round(_Ty num) noexcept
    {
        if ((static_cast<int64_t>(num * 10) % 10) >= 5) {
            return (static_cast<int64_t>(num) + 1);
        }
        return static_cast<int64_t>(num);
    }

    template <typename _Ty>
    inline _Ty Abs(_Ty value) noexcept
    {
        if (value < 0) {
            return -value;
        }
        return value;
    }

    template <typename _Ty>
    inline _Ty Square(_Ty value) noexcept
    {
        return (value * value);
    }

    template <typename _Ty>
    inline _Ty Sqrt(_Ty value, double accuracy = 0.01) noexcept
    {
        if (value == 0) {
            return 0;
        }
        auto high = static_cast<double>(value), low = 0.0;
        if (value < 1.0) {
            high = 1.0;
        }
        while ((high - low) > accuracy) {
            auto mid = (low + high) / 2;
            if ((mid * mid) > value) {
                high = mid;
            } else {
                low = mid;
            }
        }
        return static_cast<_Ty>((low + high) / 2);
    }

    template <typename _List_Ty, typename _Val_Ty>
    inline bool Contains(const _List_Ty &list, const _Val_Ty &value)
    {
        if (CU_UNLIKELY(list.begin() == list.end())) {
            return false;
        }
        return (std::find(list.begin(), list.end(), value) != list.end());
    }

    template <typename _List_Ty>
    inline typename _List_Ty::const_iterator MaxIter(const _List_Ty &list) noexcept
    {
        if (CU_UNLIKELY(list.begin() == list.end())) {
            return list.end();
        }
        auto maxIter = list.begin();
        for (auto iter = (list.begin() + 1); iter < list.end(); ++iter) {
            if (*iter > *maxIter) {
                maxIter = iter;
            }
        }
        return maxIter;
    }

    template <typename _List_Ty>
    inline typename _List_Ty::const_iterator MinIter(const _List_Ty &list) noexcept
    {
        if (CU_UNLIKELY(list.begin() == list.end())) {
            return list.end();
        }
        auto minIter = list.begin();
        for (auto iter = (list.begin() + 1); iter < list.end(); ++iter) {
            if (*iter < *minIter) {
                minIter = iter;
            }
        }
        return minIter;
    }

    template <typename _List_Ty, typename _Val_Ty>
    inline typename _List_Ty::const_iterator 
    ApproxIter(const _List_Ty &list, _Val_Ty targetVal) noexcept
    {
        if (CU_UNLIKELY(list.begin() == list.end())) {
            return list.end();
        }
        auto approxIter = list.begin();
        auto minDiff = std::abs(*approxIter - targetVal);
        for (auto iter = (list.begin() + 1); iter < list.end(); ++iter) {
            auto diff = std::abs(*iter - targetVal);
            if (diff < minDiff) {
                approxIter = iter;
                minDiff = diff;
            }
        }
        return approxIter;
    }

    template <typename _List_Ty, typename _Val_Ty>
    inline typename _List_Ty::const_iterator 
    ApproxGreaterIter(const _List_Ty &list, _Val_Ty targetVal) noexcept
    {
        if (CU_UNLIKELY(list.begin() == list.end())) {
            return list.end();
        }
        auto approxIter = list.end() - 1;
        auto minDiff = std::numeric_limits<_Val_Ty>::max();
        for (auto iter = list.begin(); iter < list.end(); ++iter) {
            auto diff = *iter - targetVal;
            if (diff >= 0 && diff < minDiff) {
                approxIter = iter;
                minDiff = diff;
            }
        }
        return approxIter;
    }

    template <typename _List_Ty, typename _Val_Ty>
    inline typename _List_Ty::const_iterator 
    ApproxLesserIter(const _List_Ty &list, _Val_Ty targetVal) noexcept
    {
        if (CU_UNLIKELY(list.begin() == list.end())) {
            return list.end();
        }
        auto approxIter = list.begin();
        auto minDiff = std::numeric_limits<_Val_Ty>::max();
        for (auto iter = list.begin(); iter < list.end(); ++iter) {
            auto diff = targetVal - *iter;
            if (diff >= 0 && diff < minDiff) {
                approxIter = iter;
                minDiff = diff;
            }
        }
        return approxIter;
    }

    template <typename _List_Ty, typename _Val_Ty>
    inline size_t ItemPos(const _List_Ty &list, _Val_Ty targetVal) noexcept
    {
        for (size_t pos = 0; pos < list.size(); pos++) {
            if (*(list.begin() + pos) == targetVal) {
                return pos;
            }
        }
        return static_cast<size_t>(-1);
    }

    template <typename _List_Ty>
    inline int64_t Average(const _List_Ty &list) noexcept
    {
        if (CU_UNLIKELY(list.begin() == list.end())) {
            return 0;
        }
        int64_t sum = 0;
        for (auto iter = list.begin(); iter < list.end(); ++iter) {
            sum += *iter;
        }
        return (sum / list.size());
    }

    template <typename _List_Ty>
    inline int64_t Sum(const _List_Ty &list) noexcept
    {
        if (CU_UNLIKELY(list.begin() == list.end())) {
            return 0;
        }
        int64_t sum = 0;
        for (auto iter = list.begin(); iter < list.end(); ++iter) {
            sum += *iter;
        }
        return sum;
    }

    template <typename _List_Ty>
    inline _List_Ty Reverse(const _List_Ty &list) 
    {
        if (CU_UNLIKELY(list.begin() == list.end())) {
            return {};
        }
        _List_Ty reversedList(list);
        std::reverse(reversedList.begin(), reversedList.end());
        return reversedList;
    }

    template <typename _List_Ty>
    inline _List_Ty Unique(const _List_Ty &list) 
    {
        if (CU_UNLIKELY(list.begin() == list.end())) {
            return {};
        }
        _List_Ty uniqueList(list);
        for (size_t pos = 0; pos < uniqueList.size(); pos++) {
            auto comp_pos = pos + 1;
            while (comp_pos < uniqueList.size()) {
                if (uniqueList[comp_pos] == uniqueList[pos]) {
                    uniqueList.erase(uniqueList.begin() + comp_pos);
                } else {
                    comp_pos++;
                }
            }
        }
        return uniqueList;
    }

    template <typename _List_Ty>
    inline _List_Ty Trim(
        const _List_Ty &list, 
        size_t max_size = std::numeric_limits<size_t>::max(), 
        int64_t min_val = std::numeric_limits<int64_t>::min(), 
        int64_t max_val = std::numeric_limits<int64_t>::max())
    {
        if (CU_UNLIKELY(list.size() == 0 || max_size == 0 || min_val > max_val)) {
            return {};
        }

        _List_Ty list_copy(list);
        std::sort(list_copy.begin(), list_copy.end());
        list_copy.erase(std::unique(list_copy.begin(), list_copy.end()), list_copy.end());
        if (list_copy.front() > max_val || list_copy.back() < min_val) {
            return {};
        }

        auto begin_iter = list_copy.end() - 1;
        for (auto iter = list_copy.begin(); iter < (list_copy.end() - 1); ++iter) {
            if (*iter >= min_val) {
                begin_iter = iter;
                break;
            }
        }
        auto end_iter = list_copy.begin();
        for (auto iter = (list_copy.end() - 1); iter > list_copy.begin(); --iter) {
            if (*iter <= max_val) {
                end_iter = iter;
                break;
            }
        }
        if (begin_iter == end_iter) {
            return _List_Ty(begin_iter, (begin_iter + 1));
        }
        if (static_cast<size_t>(end_iter - begin_iter) <= max_size) {
            return _List_Ty(begin_iter, (end_iter + 1));
        }
        if (max_size == 1) {
            auto mid_iter = begin_iter + (end_iter - begin_iter) / 2;
            return _List_Ty(mid_iter, (mid_iter + 1));
        }

        auto cur_iter = list_copy.begin();
        auto begin_val = *begin_iter;
        auto target_diff = (*end_iter - begin_val) / (max_size - 1);
        for (size_t pos = 0; pos < max_size; pos++) {
            auto target_val = begin_val + target_diff * pos;
            auto select_iter = begin_iter;
            auto min_diff = std::numeric_limits<int64_t>::max();
            for (auto iter = begin_iter; iter <= end_iter; ++iter) {
                auto diff = std::abs(static_cast<int64_t>(*iter - target_val));
                if (diff < min_diff) {
                    select_iter = iter;
                    min_diff = diff;
                } else {
                    break;
                }
            }
            cur_iter = list_copy.begin() + pos;
            *cur_iter = *select_iter;
            if (select_iter > begin_iter) {
                begin_iter = select_iter;
            }
            if (begin_iter == end_iter) {
                break;
            }
        }
        return _List_Ty(list_copy.begin(), (cur_iter + 1));
    }

    template <typename _List_Ty, typename _Elem_Ty>
    inline _List_Ty Replace(const _List_Ty &list, const _Elem_Ty &old_elem, const _Elem_Ty &new_elem)
    {
        _List_Ty new_list(list);
        std::replace(new_list.begin(), new_list.end(), old_elem, new_elem);
        return new_list;
    }

    template <typename _Item_Ty>
    inline void CreateVec_Impl(std::vector<_Item_Ty> &vec, const _Item_Ty &arg) 
    {
        vec.emplace_back(arg);
    }

    template <typename _Item_Ty, typename ..._Args>
    inline void CreateVec_Impl(std::vector<_Item_Ty> &vec, const _Item_Ty &arg, const _Args &...args) 
    {
        vec.emplace_back(arg);
        CreateVec_Impl(vec, args...);
    }

    template <typename _Item_Ty, typename ..._Args>
    inline std::vector<_Item_Ty> CreateVec(const _Item_Ty &arg, const _Args &...args) 
    {
        std::vector<_Item_Ty> vec{};
        CreateVec_Impl(vec, arg, args...);
        return vec;
    }

    template <typename _Item_Ty>
    inline std::vector<_Item_Ty> CreateVec(const _Item_Ty &arg) 
    {
        std::vector<_Item_Ty> vec{};
        CreateVec_Impl(vec, arg);
        return vec;
    }

    CU_INLINE int CompileDateCode() noexcept
    {
        static constexpr char complieDate[] = __DATE__;
    
        char month[4]{};
        int year = 0;
        int day = 0;
        (void)std::sscanf(complieDate, "%s %d %d", month, &day, &year);
        if (std::memcmp(month, "Jan", 3) == 0) {
            return (year * 10000 + 100 + day);
        }
        if (std::memcmp(month, "Feb", 3) == 0) {
            return (year * 10000 + 200 + day);
        }
        if (std::memcmp(month, "Mar", 3) == 0) {
            return (year * 10000 + 300 + day);
        }
        if (std::memcmp(month, "Apr", 3) == 0) {
            return (year * 10000 + 400 + day);
        }
        if (std::memcmp(month, "May", 3) == 0) {
            return (year * 10000 + 500 + day);
        }
        if (std::memcmp(month, "Jun", 3) == 0) {
            return (year * 10000 + 600 + day);
        }
        if (std::memcmp(month, "Jul", 3) == 0) {
            return (year * 10000 + 700 + day);
        }
        if (std::memcmp(month, "Aug", 3) == 0) {
            return (year * 10000 + 800 + day);
        }
        if (std::memcmp(month, "Sep", 3) == 0) {
            return (year * 10000 + 900 + day);
        }
        if (std::memcmp(month, "Oct", 3) == 0) {
            return (year * 10000 + 1000 + day);
        }
        if (std::memcmp(month, "Nov", 3) == 0) {
            return (year * 10000 + 1100 + day);
        }
        if (std::memcmp(month, "Dec", 3) == 0) {
            return (year * 10000 + 1200 + day);
        }
        return -1;
    }

    CU_INLINE time_t TimeStamp()
    {
        auto time_pt = std::chrono::system_clock::now();
        auto time_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(time_pt);
        return static_cast<time_t>(time_ms.time_since_epoch().count());
    }

    CU_INLINE void SleepMs(time_t time) 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(time));
    }

    CU_INLINE int RunCommand(const std::string &command) noexcept
    {
        return std::system(command.data());
    }

    CU_INLINE void Pause()
    {
        for (;;) {
            std::this_thread::sleep_for(std::chrono::seconds(std::numeric_limits<time_t>::max()));
        }
    }
}

#endif // __LIB_CU__
