#include <iostream>
#include "CuLogger.h"

#define CLOGE(...) CU::Logger::Error(__VA_ARGS__)
#define CLOGW(...) CU::Logger::Warn(__VA_ARGS__)
#define CLOGI(...) CU::Logger::Info(__VA_ARGS__)
#define CLOGD(...) CU::Logger::Debug(__VA_ARGS__)
#define CLOGV(...) CU::Logger::Verbose(__VA_ARGS__)

int main(int argc, char* argv[])
{
    std::string logPath = argv[0];
    if (logPath.rfind("\\") != std::string::npos) {
        logPath = logPath.substr(0, logPath.rfind("\\")) + "\\log.txt";
    } else if (logPath.rfind("/") != std::string::npos) {
        logPath = logPath.substr(0, logPath.rfind("/")) + "/log.txt";
    }
    
    CU::Logger::Create(CU::Logger::LogLevel::INFO, logPath);

    CLOGE("This is log output.");
    CLOGW("This is log output.");
    CLOGI("This is log output.");
    CLOGD("This is log output.");
    CLOGV("This is log output.");

    std::thread thread0([&]() {
        for (int i = 1; i <= 1000000; i++) {
            CLOGI("thread0 log %d.", i);
        }
        CU::Logger::Flush();
        std::exit(0);
    });
    thread0.detach();

    std::thread thread1([&]() {
        for (int i = 1; i <= 1000000; i++) {
            CLOGI("thread1 log %d.", i);
        }
    });
    thread1.detach();
    
    CLOGI("MainThread waiting.");

    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(INT_MAX));
    }

    return 0;
}
