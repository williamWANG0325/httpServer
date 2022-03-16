#pragma once

#include "BlockQueue.h"
#include <thread>
#include <string>
#include <atomic>
#include <mutex>
#include <cstdarg>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

class Log
{
public:
    static Log* getInstence();
    void openLog(int max_buffer_size = 1024 );//线程不安全
    void closeLog();//线程不安全
    void writeLog(int level, const char* format, ...);
    ~Log();

private:
    Log();
    void writeLogToFile();

private:
    std::atomic_bool alive;

    FILE* filePtr;
    char logName[256];
    char dirName[256];
//    int maxLine;
//    long long countLine;

    BlockQueue<std::string> logQueue;
    
    int maxBufferSize;
    std::mutex mutexLock;

    std::atomic_int today;
    std::thread writer;
};

#define LOG_BASE(level, format, ...) \
    do {Log::getInstence()->writeLog(level, format, ##__VA_ARGS__);} while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0)
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0)
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0)
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0)

