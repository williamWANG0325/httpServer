#pragma once

#include "../log_system/Log.h"

#include <arpa/inet.h>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdarg>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>

class HttpConnection
{
public:
    enum LINE_STATUS {
        LINE_OK,
        LINE_BAD,
        LINE_OPEN
    };
    enum METHOD {
        GET,
        POST
    };
    enum HTTP_CODE{
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        INCOMPLETE_WRITE,
        KEEP_ALIVE,
        COMPLETE_WRITE
    };
    enum CHECK_STATE{
        CHECK_STATE_REQUESTLINE,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    static std::atomic_int totalConnection;
    static char rootPath[100];
    static std::atomic_bool isET;


public:
    HttpConnection() ;
    ~HttpConnection() {};

    void init(int sockFd, const sockaddr_in & addr);

    static void initStatic(const char* root, bool ET);

    void closeConnection();

    HTTP_CODE processRead();
    HTTP_CODE processWrite();
    
private:
    void init();
    
    bool readToBuffer();
    
    LINE_STATUS parseLine();

    HTTP_CODE parseRequestLine(char *text);
    HTTP_CODE parseHeaders(char* text);
    HTTP_CODE parseContent(char* text);
    HTTP_CODE parse();

    HTTP_CODE doRequest();

    bool fillWriteBuffer(HTTP_CODE ret);
    bool addResponse(const char* format, ...) ;
    bool addStatusLine(int status, const char* title);
    bool addContentType(char* s = nullptr) ;
    bool addHeaders(int responseLen);
    
 
    void unmap();

private:
    int fd;
    sockaddr_in sockAddr;
    
    bool alive;

    char readBuffer[READ_BUFFER_SIZE];
    int readIdx;
    int checkedIdx;
    int parsedIdx;


    char writeBuffer[WRITE_BUFFER_SIZE];
    int writeIdx;
    int bytesHaveSend;
    int bytesToSend;

    struct iovec writeIV[2];
    int ivCount;


    char requestFile[FILENAME_LEN];

    CHECK_STATE masterState;

    METHOD method;

    char* url;
    char* version;
    char* host;
    char* content;
    int contentLen;
    bool keepAlive;
    struct stat fileStat;
    char * mmapWriteFile;


};

