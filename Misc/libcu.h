#ifndef __LIB_CU__
#define __LIB_CU__

#include <iostream>
#include <stdexcept>
#include <utility>
#include <string>
#include <vector>
#include <array>
#include <thread>
#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include <limits>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cstdarg>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <cctype>
#include <cinttypes>

#define CU_UNUSED(val) (void)(val)
#define CU_WCHAR(val) L##val

#if defined(__GNUC__)
#define CU_INLINE __attribute__((always_inline)) inline
#define CU_LIKELY(val) (__builtin_expect(!!(val), 1))
#define CU_UNLIKELY(val) (__builtin_expect(!!(val), 0))
#define CU_COMPARE(val1, val2, size) (__builtin_memcmp(val1, val2, size) == 0)
#define CU_MEMSET(dst, ch, size) __builtin_memset(dst, ch, size)
#elif defined(_MSC_VER)
#define CU_INLINE __forceinline
#define CU_LIKELY(val) (!!(val))
#define CU_UNLIKELY(val) (!!(val))
#define CU_COMPARE(val1, val2, size) (memcmp(val1, val2, size) == 0)
#define CU_MEMSET(dst, ch, size) memset(dst, ch, size)
#else
#define CU_INLINE inline
#define CU_LIKELY(val) (!!(val))
#define CU_UNLIKELY(val) (!!(val))
#define CU_COMPARE(val1, val2, size) (memcmp(val1, val2, size) == 0)
#define CU_MEMSET(dst, ch, size) memset(dst, ch, size)
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


CU_INLINE std::vector<std::string> StrSplit(const std::string &str, const std::string &delimiter)
{
    std::vector<std::string> strList{};
    size_t start_pos = 0;
    size_t pos = str.find(delimiter);
    while (pos != std::string::npos) {
        if (start_pos < pos) {
            strList.emplace_back(str.substr(start_pos, pos - start_pos));
        }
        start_pos = pos + delimiter.size();
        pos = str.find(delimiter, start_pos);
    }
    if (start_pos < str.size()) {
        strList.emplace_back(str.substr(start_pos));
    }
    return strList;
}

CU_INLINE std::vector<std::string> StrSplit(const std::string &str, char delimiter) 
{
    std::vector<std::string> lines{};
    size_t start_pos = 0;
    for (size_t pos = 0; pos < str.size(); pos++) {
        if (str[pos] == delimiter) {
            if (start_pos < pos) {
                lines.emplace_back(str.substr(start_pos, pos - start_pos));
            }
            start_pos = pos + 1;
        }
    }
    if (start_pos < str.size()) {
        lines.emplace_back(str.substr(start_pos));
    }
    return lines;
}

CU_INLINE std::string StrSplitAt(const std::string &str, const std::string &delimiter, int targetCount) 
{
    int count = 0;
    size_t start_pos = 0;
    size_t pos = str.find(delimiter);
    while (pos != std::string::npos) {
        if (start_pos < pos) {
            if (count == targetCount) {
                return str.substr(start_pos, pos - start_pos);
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

CU_INLINE std::string StrSplitAt(const std::string &str, char delimiter, int targetCount) 
{
    int count = 0;
    size_t start_pos = 0;
    for (size_t pos = 0; pos < str.size(); pos++) {
        if (str[pos] == delimiter) {
            if (start_pos < pos) {
                if (count == targetCount) {
                    return str.substr(start_pos, pos - start_pos);
                }
                count++;
            }
            start_pos = pos + 1;
        }
    }
    if (start_pos < str.size() && count == targetCount) {
        return str.substr(start_pos);
    }
    return {};
}

CU_INLINE std::string StrMerge(const char* format, ...)
{
    std::string content{};
    int len = 0;
    {
        va_list args{};
        va_start(args, format);
        len = std::vsnprintf(nullptr, 0, format, args) + 1;
        va_end(args);
    }
    if (CU_LIKELY(len > 1)) {
        auto buffer = new char[len];
        CU_MEMSET(buffer, 0, len);
        va_list args{};
        va_start(args, format);
        std::vsnprintf(buffer, len, format, args);
        va_end(args);
        content = buffer;
        delete[] buffer;
    }
    return content;
}

CU_INLINE std::string GetPrevString(const std::string &str, const std::string &key)
{
    return str.substr(0, str.find(key));
}

CU_INLINE std::string GetRePrevString(const std::string &str, const std::string &key)
{
    return str.substr(0, str.rfind(key));
}

CU_INLINE std::string GetPostString(const std::string &str, const std::string &key)
{
    return str.substr(str.find(key) + key.size());
}

CU_INLINE std::string GetRePostString(const std::string &str, const std::string &key)
{
    return str.substr(str.rfind(key) + key.size());
}

CU_INLINE bool StrContains(const std::string &str, const std::string &subStr) noexcept
{
    return (str.find(subStr) != std::string::npos);
}

CU_INLINE int StringToInteger(const std::string &str) noexcept
{
    char buffer[32] = { 0 };
    int buffer_offset = 0;
    for (int pos = 0; pos < str.size() && pos < (sizeof(buffer) - 1); pos++) {
        if (str[pos] >= '0' && str[pos] <= '9') {
            buffer[buffer_offset] = str[pos];
            buffer_offset++;
        }
    }
    buffer[buffer_offset] = '\0';
    return std::atoi(buffer);
}

CU_INLINE int64_t StringToLong(const std::string &str) noexcept
{
    char buffer[32] = { 0 };
    int buffer_offset = 0;
    for (int pos = 0; pos < str.size() && pos < (sizeof(buffer) - 1); pos++) {
        if (str[pos] >= '0' && str[pos] <= '9') {
            buffer[buffer_offset] = str[pos];
            buffer_offset++;
        }
    }
    buffer[buffer_offset] = '\0';
    return std::atoll(buffer);
}

CU_INLINE double StringToDouble(const std::string &str) noexcept
{
    char buffer[32] = { 0 };
    int buffer_offset = 0;
    for (int pos = 0; pos < str.size() && pos < (sizeof(buffer) - 1); pos++) {
        if ((str[pos] >= '0' && str[pos] <= '9') || str[pos] == '.') {
            buffer[buffer_offset] = str[pos];
            buffer_offset++;
        }
    }
    buffer[buffer_offset] = '\0';
    return std::atof(buffer);
}

CU_INLINE uint64_t String16BitToInteger(const std::string &str) noexcept
{
    char buffer[32] = { 0 };
    int buffer_offset = 0;
    for (int pos = 0; pos < str.size() && pos < (sizeof(buffer) - 1); pos++) {
        if ((str[pos] >= '0' && str[pos] <= '9') || 
            (str[pos] >= 'a' && str[pos] <= 'f') || 
            (str[pos] >= 'A' && str[pos] <= 'F')
        ) {
            buffer[buffer_offset] = str[pos];
            buffer_offset++;
        }
    }
    buffer[buffer_offset] = '\0';
    return std::strtoull(buffer, nullptr, 16);
}

CU_INLINE std::string TrimStr(const std::string &str) 
{
    std::string trimedStr{};
    for (const auto &ch : str) {
        switch (ch) {
            case ' ':
            case '\n':
            case '\t':
            case '\r':
            case '\f':
            case '\a':
            case '\b':
            case '\v':
                break;
            default:
                trimedStr += ch;
                break;
        }
    }
    return trimedStr;
}

template <typename _Ty>
CU_INLINE size_t GetHash(const _Ty &val)
{
    std::hash<_Ty> hashVal{};
    return hashVal(val);
}

template <typename _Ty>
CU_INLINE bool GenericCompare(const _Ty &val0, const _Ty &val1) noexcept
{
    const auto val0_addr = std::addressof(val0);
    const auto val1_addr = std::addressof(val1);
    if (CU_UNLIKELY(val0_addr == val1_addr)) {
        return true;
    }
    return CU_COMPARE(val0_addr, val1_addr, sizeof(_Ty));
}

template <typename _Ty>
CU_INLINE int RoundNum(_Ty num) noexcept
{
    if (num > INT_MAX) {
        return INT_MAX;
    } else if (num < INT_MIN) {
        return INT_MIN;
    }
    int intNum = 0;
    int dec = static_cast<int>(num * 10) % 10;
    if (dec >= 5) {
        intNum = static_cast<int>(num) + 1;
    } else {
        intNum = static_cast<int>(num);
    }
    return intNum;
}

template <typename _Ty>
CU_INLINE _Ty AbsVal(_Ty num) noexcept
{
    if (num < 0) {
        return -num;
    }
    return num;
}

template <typename _Ty>
CU_INLINE _Ty SquareVal(_Ty value) noexcept
{
    return (value * value);
}

template <typename _Ty>
CU_INLINE _Ty SqrtVal(_Ty value) noexcept
{
    if (value == 0) {
        return 0;
    }
    auto high = static_cast<double>(value), low = 0.0;
    if (value < 1.0) {
        high = 1.0;
    }
    while ((high - low) > 0.01) {
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
CU_INLINE bool ListContains(const _List_Ty &list, const _Val_Ty &value)
{
    return (std::find(list.begin(), list.end(), value) != list.end());
}

template <typename _Ty>
CU_INLINE const _Ty &VecMaxItem(const std::vector<_Ty> &vec) noexcept
{
    auto maxIter = vec.begin();
    for (auto iter = vec.begin(); iter < vec.end(); iter++) {
        if (*iter > *maxIter) {
            maxIter = iter;
        }
    }
    return *maxIter;
}

template <typename _Ty>
CU_INLINE const _Ty &VecMinItem(const std::vector<_Ty> &vec) noexcept
{
    auto minIter = vec.begin();
    for (auto iter = vec.begin(); iter < vec.end(); iter++) {
        if (*iter < *minIter) {
            minIter = iter;
        }
    }
    return *minIter;
}

template <typename _Ty>
CU_INLINE const _Ty &VecApproxItem(const std::vector<_Ty> &vec, _Ty targetVal) noexcept
{
    auto approxIter = vec.begin();
    _Ty minDiff = std::numeric_limits<_Ty>::max();
    for (auto iter = vec.begin(); iter < vec.end(); iter++) {
        _Ty diff = std::abs(*iter - targetVal);
        if (diff < minDiff) {
            approxIter = iter;
            minDiff = diff;
        }
    }
    return *approxIter;
}

template <typename _Ty>
CU_INLINE const _Ty &VecApproxMinItem(const std::vector<_Ty> &vec, _Ty targetVal) noexcept
{
    auto approxIter = vec.end() - 1;
    _Ty minDiff = std::numeric_limits<_Ty>::max();
    for (auto iter = (vec.end() - 1); iter >= vec.begin(); iter--) {
        _Ty diff = *iter - targetVal;
        if (diff < minDiff && diff >= 0) {
            approxIter = iter;
            minDiff = diff;
        }
    }
    return *approxIter;
}

template <typename _Ty>
CU_INLINE const _Ty &VecApproxMaxItem(const std::vector<_Ty> &vec, _Ty targetVal) noexcept
{
    auto approxIter = vec.begin();
    _Ty minDiff = std::numeric_limits<_Ty>::max();
    for (auto iter = vec.begin(); iter < vec.end(); iter++) {
        _Ty diff = targetVal - *iter;
        if (diff < minDiff && diff >= 0) {
            approxIter = iter;
            minDiff = diff;
        }
    }
    return *approxIter;
}

template <typename _Ty>
CU_INLINE _Ty AverageVec(const std::vector<_Ty> &vec) noexcept
{
    _Ty sum{};
    for (auto iter = vec.begin(); iter < vec.end(); iter++) {
        sum += *iter;
    }
    return (sum / vec.size());
}

template <typename _Ty>
CU_INLINE _Ty SumVec(const std::vector<_Ty> &vec) noexcept
{
    _Ty sum{};
    for (auto iter = vec.begin(); iter < vec.end(); iter++) {
        sum += *iter;
    }
    return sum;
}

template <typename _Ty>
CU_INLINE std::vector<_Ty> UniqueVec(const std::vector<_Ty> &vec) 
{
    std::vector<_Ty> uniquedVec(vec);
    std::sort(uniquedVec.begin(), uniquedVec.end());
    auto iter = std::unique(uniquedVec.begin(), uniquedVec.end());
    uniquedVec.erase(iter, uniquedVec.end());
    return uniquedVec;
}

template <typename _Ty>
CU_INLINE std::vector<_Ty> ShrinkVec(const std::vector<_Ty> &vec, size_t size) 
{
    std::vector<_Ty> trimedVec(vec);
    std::sort(trimedVec.begin(), trimedVec.end());
    auto iter = std::unique(trimedVec.begin(), trimedVec.end());
    trimedVec.erase(iter, trimedVec.end());
    if (trimedVec.size() <= size) {
        return trimedVec;
    }
    std::vector<_Ty> shrinkedVec{};
    _Ty itemDiff = (trimedVec.back() - trimedVec.front()) / (size - 1);
    for (size_t idx = 0; idx < size; idx++) {
        _Ty selectVal = trimedVec.front() + itemDiff * idx;
        auto selectIter = trimedVec.begin();
        _Ty minDiff = trimedVec.back();
        for (auto iter = trimedVec.begin(); iter < trimedVec.end(); iter++) {
            _Ty diff = std::abs(*iter - selectVal);
            if (diff < minDiff) {
                selectIter = iter;
                minDiff = diff;
            } else {
                break;
            }
        }
        shrinkedVec.emplace_back(*selectIter);
    }
    return shrinkedVec;
}

template <typename _Ty>
CU_INLINE std::vector<_Ty> ReverseVec(const std::vector<_Ty> &vec) 
{
    std::vector<_Ty> reversedVec(vec);
    std::reverse(reversedVec.begin(), reversedVec.end());
    return reversedVec;
}

CU_INLINE int GetCompileDateCode() noexcept
{
    static constexpr char complieDate[] = __DATE__;
    
    char month[4] = { 0 };
    int year = 0;
    int day = 0;
    std::sscanf(complieDate, "%s %d %d", month, &day, &year);
    if (CU_COMPARE(month, "Jan", 3)) {
        return (year * 10000 + 100 + day);
    }
    if (CU_COMPARE(month, "Feb", 3)) {
        return (year * 10000 + 200 + day);
    }
    if (CU_COMPARE(month, "Mar", 3)) {
        return (year * 10000 + 300 + day);
    }
    if (CU_COMPARE(month, "Apr", 3)) {
        return (year * 10000 + 400 + day);
    }
    if (CU_COMPARE(month, "May", 3)) {
        return (year * 10000 + 500 + day);
    }
    if (CU_COMPARE(month, "Jun", 3)) {
        return (year * 10000 + 600 + day);
    }
    if (CU_COMPARE(month, "Jul", 3)) {
        return (year * 10000 + 700 + day);
    }
    if (CU_COMPARE(month, "Aug", 3)) {
        return (year * 10000 + 800 + day);
    }
    if (CU_COMPARE(month, "Sep", 3)) {
        return (year * 10000 + 900 + day);
    }
    if (CU_COMPARE(month, "Oct", 3)) {
        return (year * 10000 + 1000 + day);
    }
    if (CU_COMPARE(month, "Nov", 3)) {
        return (year * 10000 + 1100 + day);
    }
    if (CU_COMPARE(month, "Dec", 3)) {
        return (year * 10000 + 1200 + day);
    }
    return -1;
}

CU_INLINE time_t GetTimeStampMs()
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
    return std::system(command.c_str());
}

CU_INLINE void Pause()
{
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(INT64_MAX));
    }
}


#if defined(__unix__)

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>

CU_INLINE void CreateFile(const std::string &filePath, const std::string &content) noexcept
{
    int fd = open(filePath.c_str(), (O_WRONLY | O_CREAT | O_TRUNC), 0644);
    if (CU_LIKELY(fd >= 0)) {
        write(fd, content.c_str(), (content.length() + 1));
        close(fd);
    }
}

CU_INLINE void AppendFile(const std::string &filePath, const std::string &content) noexcept
{
    int fd = open(filePath.c_str(), (O_WRONLY | O_APPEND | O_NONBLOCK));
    if (fd < 0) {
        chmod(filePath.c_str(), 0666);
        fd = open(filePath.c_str(), (O_WRONLY | O_APPEND | O_NONBLOCK));
    }
    if (CU_LIKELY(fd >= 0)) {
        write(fd, content.c_str(), (content.length() + 1));
        close(fd);
    }
}

CU_INLINE void WriteFile(const std::string &filePath, const std::string &content) noexcept
{
    int fd = open(filePath.c_str(), (O_WRONLY | O_NONBLOCK));
    if (fd < 0) {
        chmod(filePath.c_str(), 0666);
        fd = open(filePath.c_str(), (O_WRONLY | O_NONBLOCK));
    }
    if (CU_LIKELY(fd >= 0)) {
        write(fd, content.c_str(), (content.length() + 1));
        close(fd);
    }
}

CU_INLINE std::string ReadFile(const std::string &filePath) 
{
    std::string content{};
    int fd = open(filePath.c_str(), (O_RDONLY | O_NONBLOCK));
    if (CU_LIKELY(fd >= 0)) {
        char buffer[PAGE_SIZE] = { 0 };
        while (read(fd, buffer, (sizeof(buffer) - 1)) > 0) {
            content += buffer;
            CU_MEMSET(buffer, 0, sizeof(buffer));
        }
        close(fd);
    }
    return content;
}

CU_INLINE bool IsPathExists(const std::string &path) noexcept
{
    struct stat buffer{};
    return (lstat(path.c_str(), std::addressof(buffer)) == 0);
}

CU_INLINE int GetThreadPid(int tid) noexcept
{
    int pid = -1;
    char statusPath[PATH_MAX] = { 0 };
    std::snprintf(statusPath, sizeof(statusPath), "/proc/%d/status", tid);
    int fd = open(statusPath, (O_RDONLY | O_NONBLOCK));
    if (CU_LIKELY(fd >= 0)) {
        char buffer[PAGE_SIZE] = { 0 };
        if (read(fd, buffer, (sizeof(buffer) - 1)) > 0) {
            std::sscanf(std::strstr(buffer, "Tgid:"), "Tgid: %d", &pid);
        }
        close(fd);
    }
    return pid;
}

CU_INLINE std::string GetTaskCgroupType(int pid, const std::string &cgroup)
{
    std::string cgroupContent{};
    {
        char cgroupPath[PATH_MAX] = { 0 };
        std::snprintf(cgroupPath, sizeof(cgroupPath), "/proc/%d/cgroup", pid);
        int fd = open(cgroupPath, (O_RDONLY | O_NONBLOCK));
        if (CU_LIKELY(fd >= 0)) {
            char buffer[PAGE_SIZE] = { 0 };
            if (read(fd, buffer, (sizeof(buffer) - 1)) > 0) {
                cgroupContent = buffer;
            }
            close(fd);
        }
    }
    if (!cgroupContent.empty()) {
        auto begin_pos = cgroupContent.find(cgroup);
        if (begin_pos != std::string::npos) {
            begin_pos = begin_pos + cgroup.length() + 1;
            auto end_pos = cgroupContent.find('\n', begin_pos);
            if (end_pos == std::string::npos) {
                end_pos = cgroupContent.length();
            }
            return cgroupContent.substr(begin_pos, end_pos - begin_pos);
        }
    }
    return cgroupContent;
}

CU_INLINE std::string GetTaskName(int pid) 
{
    std::string taskName{};
    char cmdlinePath[PATH_MAX] = { 0 };
    std::snprintf(cmdlinePath, sizeof(cmdlinePath), "/proc/%d/cmdline", pid);
    int fd = open(cmdlinePath, (O_RDONLY | O_NONBLOCK));
    if (CU_LIKELY(fd >= 0)) {
        char buffer[PAGE_SIZE] = { 0 };
        if (read(fd, buffer, (sizeof(buffer) - 1)) > 0) {
            taskName = buffer;
        }
        close(fd);
    }
    return taskName;
}

CU_INLINE std::string GetTaskComm(int pid) 
{
    std::string taskComm{};
    char commPath[PATH_MAX] = { 0 };
    std::snprintf(commPath, sizeof(commPath), "/proc/%d/comm", pid);
    int fd = open(commPath, (O_RDONLY | O_NONBLOCK));
    if (CU_LIKELY(fd >= 0)) {
        char buffer[PAGE_SIZE] = { 0 };
        if (read(fd, buffer, (sizeof(buffer) - 1)) > 0) {
            taskComm = buffer;
        }
        close(fd);
    }
    return taskComm;
}

CU_INLINE uint64_t GetThreadRuntime(int pid, int tid) noexcept
{
    uint64_t runtime = 0;
    char statPath[PATH_MAX] = { 0 };
    std::snprintf(statPath, sizeof(statPath), "/proc/%d/task/%d/stat", pid, tid);
    int fd = open(statPath, (O_RDONLY | O_NONBLOCK));
    if (CU_LIKELY(fd >= 0)) {
        char buffer[PAGE_SIZE] = { 0 };
        if (read(fd, buffer, (sizeof(buffer) - 1)) > 0) {
            uint64_t utime = 0, stime = 0;
            std::sscanf(strchr(buffer, ')') + 2, "%*c %*ld %*ld %*ld %*ld %*ld %*ld %*ld %*ld %*ld %*ld %" SCNu64 " %" SCNu64,
            &utime, &stime);
            runtime = utime + stime;
        }
        close(fd);
    }
    return runtime;
}

CU_INLINE void SetThreadName(const std::string &name) noexcept
{
    prctl(PR_SET_NAME, name.c_str());
}

CU_INLINE int FindTaskPid(const std::string &taskName)
{
    std::vector<int> tasks{};
    struct dirent** entries = nullptr;
    int size = scandir("/proc", &entries, nullptr, alphasort);
    if (CU_LIKELY(entries != nullptr)) {
        for (int offset = 0; offset < size; offset++) {
            auto entry = *(entries + offset);
            auto pid = std::atoi(entry->d_name);
            if (entry->d_type == DT_DIR && pid > 0 && pid <= INT16_MAX) {
                tasks.emplace_back(pid);
            }
            std::free(entry);
        }
        std::free(entries);
    }
    for (const int &pid : tasks) {
        char cmdlinePath[PATH_MAX] = { 0 };
        std::snprintf(cmdlinePath, sizeof(cmdlinePath), "/proc/%d/cmdline", pid);
        int fd = open(cmdlinePath, (O_RDONLY | O_NONBLOCK));
        if (CU_LIKELY(fd >= 0)) {
            char cmdline[PAGE_SIZE] = { 0 };
            read(fd, cmdline, (sizeof(cmdline) - 1));
            close(fd);
            if (std::strstr(cmdline, taskName.c_str())) {
                return pid;
            }
        }
    }
    return -1;
}

CU_INLINE std::vector<int> GetTaskThreads(int pid) 
{
    std::vector<int> threads{};
    char taskPath[PATH_MAX] = { 0 };
    std::snprintf(taskPath, sizeof(taskPath), "/proc/%d/task", pid);
    struct dirent** entries = nullptr;
    int size = scandir(taskPath, &entries, nullptr, alphasort);
    if (CU_LIKELY(entries != nullptr)) {
        for (int offset = 0; offset < size; offset++) {
            auto entry = *(entries + offset);
            int tid = std::atoi(entry->d_name);
            if (entry->d_type == DT_DIR && tid > 0 && tid <= INT16_MAX) {
                threads.emplace_back(tid);
            }
            std::free(entry);
        }
        std::free(entries);
    }
    return threads;
}

CU_INLINE int GetLinuxKernelVersion() noexcept
{
    int version = 0;
    struct utsname uts{};
    if (uname(&uts) != -1) {
        int r = 0, x = 0, y = 0;
        std::sscanf(uts.release, "%d.%d.%d", &r, &x, &y);
        version = r * 100000 + x * 1000 + y;
    }
    return version;
}

CU_INLINE std::string ExecCommand(const std::string &command)
{
    std::string content{};
    auto fp = popen(command.c_str(), "r");
    if (CU_LIKELY(fp != nullptr)) {
        char buffer[PAGE_SIZE] = { 0 };
        while (std::fread(buffer, sizeof(char), (sizeof(buffer) - 1), fp) > 0) {
            content += buffer;
            CU_MEMSET(buffer, 0, sizeof(buffer));
        }
        pclose(fp);
    }
    return content;
}

CU_INLINE std::string FindPath(const std::string &path, const std::string &symbol)
{
    std::vector<std::string> paths{};
    struct dirent** entries = nullptr;
    int size = scandir(path.c_str(), &entries, nullptr, alphasort);
    if (CU_LIKELY(entries != nullptr)) {
        for (int offset = 0; offset < size; offset++) {
            auto entry = *(entries + offset);
            paths.emplace_back(entry->d_name);
            std::free(entry);
        }
        std::free(entries);
    }
    for (const auto &path : paths) {
        if (path.find(symbol) != std::string::npos) {
            return path;
        }
    }
    return {};
}

CU_INLINE std::vector<std::string> ListDir(const std::string &path) 
{
    std::vector<std::string> dirPaths{};
    struct dirent** entries = nullptr;
    int size = scandir(path.c_str(), &entries, nullptr, alphasort);
    if (CU_LIKELY(entries != nullptr)) {
        for (int offset = 0; offset < size; offset++) {
            auto entry = *(entries + offset);
            if (entry->d_type == DT_DIR) {
                dirPaths.emplace_back(path + '/' + entry->d_name);
            }
            std::free(entry);
        }
        std::free(entries);
    }
    return dirPaths;
}

CU_INLINE std::vector<std::string> ListFile(const std::string &path, uint8_t d_type = DT_REG)
{
    std::vector<std::string> filePaths{};
    struct dirent** entries = nullptr;
    int size = scandir(path.c_str(), &entries, nullptr, alphasort);
    if (CU_LIKELY(entries != nullptr)) {
        for (int offset = 0; offset < size; offset++) {
            auto entry = *(entries + offset);
            if (entry->d_type == d_type) {
                filePaths.emplace_back(path + '/' + entry->d_name);
            }
            std::free(entry);
        }
        std::free(entries);
    }
    return filePaths;
}

#if defined(__ANDROID_API__)

#include <sys/system_properties.h>

CU_INLINE std::string DumpTopActivityInfo() 
{
    std::string activityInfo{};
    auto fp = popen("dumpsys activity oom 2>/dev/null", "r");
    if (CU_LIKELY(fp != nullptr)) {
        char buffer[PAGE_SIZE]= { 0 };
        while (std::fgets(buffer, sizeof(buffer), fp) != nullptr) {
            if (std::strstr(buffer, "top-activity")) {
                activityInfo = buffer;
                break;
            }
        }
        pclose(fp);
    }
    return activityInfo;
}

enum class ScreenState : uint8_t {SCREEN_OFF, SCREEN_ON};

CU_INLINE ScreenState GetScreenStateViaCgroup() noexcept
{
    ScreenState state = ScreenState::SCREEN_ON;
    int fd = open("/dev/cpuset/restricted/tasks", (O_RDONLY | O_NONBLOCK));
    if (CU_LIKELY(fd >= 0)) {
        char buffer[PAGE_SIZE] = { 0 };
        if (read(fd, buffer, (sizeof(buffer) - 1)) > 0) {
            int taskCount = 0;
            for (int pos = 0; pos < sizeof(buffer); pos++) {
                if (buffer[pos] == '\n') {
                    taskCount++;
                } else if (buffer[pos] == 0) {
                    break;
                }
            }
            if (taskCount > 10) {
                state = ScreenState::SCREEN_OFF;
            }
        }
        close(fd);
    }
    return state;
}

CU_INLINE ScreenState GetScreenStateViaWakelock() noexcept
{
    ScreenState state = ScreenState::SCREEN_ON;
    int fd = open("/sys/power/wake_lock", (O_RDONLY | O_NONBLOCK));
    if (CU_LIKELY(fd >= 0)) {
        char buffer[PAGE_SIZE] = { 0 };
        if (read(fd, buffer, (sizeof(buffer) - 1)) > 0) {
            if (!std::strstr(buffer, "PowerManagerService.Display")) {
                state = ScreenState::SCREEN_OFF;
            }
        }
        close(fd);
    }
    return state;
}

CU_INLINE int GetAndroidAPILevel() noexcept
{
    return android_get_device_api_level();
}


#endif // __ANDROID_API__
#endif // __unix__
#endif // __LIB_CU__
