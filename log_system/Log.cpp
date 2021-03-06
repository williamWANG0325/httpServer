#include "Log.h"

Log*  Log::getInstence() {
    static Log myLog;
    return &myLog;
}

void Log::openLog(int max_buffer_size) {
    if (alive) {
        return;
    }
    
    maxBufferSize = max_buffer_size;
    
    writer = std::thread(std::bind(&Log::writeLogToFile,this));

    memset(logName, 0, sizeof(logName));
    strcpy(dirName, "./log");

    time_t t = time(nullptr);
    struct tm *cntTime = localtime(&t);

    snprintf(logName, sizeof(logName)-1, "./log/%d_%02d_%02d.log", cntTime->tm_year + 1900, cntTime->tm_mon + 1, cntTime->tm_mday);
    
    today.store(cntTime->tm_mday);

    DIR* dir = opendir(dirName);
    if (dir == nullptr)
        mkdir("./log", 0755);
    closedir(dir);
    filePtr = fopen(logName, "a");
    alive = true;
}


void Log::writeLogToFile() {
    std::string singleLog;
    while (logQueue.get(singleLog)) {
        std::unique_lock<std::mutex> myLock{mutexLock};
        fputs(singleLog.c_str(), filePtr);
        //fflush(filePtr);
    }
}

void Log::writeLog(int level, const char* format, ...) {
    if(!alive)
        return;

    time_t t = time(nullptr);
    struct tm *cntTime = localtime(&t);

    char levelString[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(levelString,  "[debug]:");
        break;
    case 1:
        strcpy(levelString, "[info]:");
        break;
    case 2:
        strcpy(levelString, "[warn]:");
        break;
    case 3:
        strcpy(levelString, "[erro]:");
        break;
    default:
        strcpy(levelString, "[info]:");
        break;
    }

    va_list valst;
    va_start(valst, format);

    char *buffer = new char[maxBufferSize];
    memset(buffer, 0, maxBufferSize);
    
    if (today != cntTime->tm_mday) {
        std::unique_lock<std::mutex> myLock{mutexLock};
        if (today != cntTime->tm_mday) {
            snprintf(logName, sizeof(logName)-1, "./log/%d_%02d_%02d.log", cntTime->tm_year + 1900, cntTime->tm_mon + 1, cntTime->tm_mday);
            today.store(cntTime->tm_mday);
            fclose(filePtr);
            filePtr = fopen(logName, "a");
        }
    }

    int len1 = snprintf(buffer, 48, "%d-%02d-%02d %02d:%02d:%02d %s ",
                 cntTime->tm_year + 1900, cntTime->tm_mon + 1, cntTime->tm_mday,
                 cntTime->tm_hour, cntTime->tm_min, cntTime->tm_sec, levelString);
    int len2 = vsnprintf(buffer + len1, maxBufferSize - 1, format, valst);
    buffer[len1 + len2] = '\n';
    buffer[len1 + len2] = '\0';
    logQueue.push(std::string(buffer));
    delete []buffer;

}

Log::Log():alive(false){}

void Log::closeLog(){
    alive = false;
    logQueue.destroyNow();
    if (writer.joinable())
        writer.join();
}

Log::~Log(){
    closeLog();
}
