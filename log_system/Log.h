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
    
//    char* buffer;
    int maxBufferSize;
    std::mutex mutexLock;

    std::atomic_int today;
    std::thread writer;


};

