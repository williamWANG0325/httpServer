#pragma once

#include "../log_system/Log.h"

#include <arpa/inet.h>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>

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
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    static std::atomic_int totalConnection;
    static std::atomic_int epollFd;

public:
    HttpConnection() ;
    ~HttpConnection() ;

    void init(int epollFd_, int sockFd, const sockaddr_in & addr);
    void closeConnection();
    
    LINE_STATUS parseLine();
    bool readToBuffer();
    
    HTTP_CODE parseRequestLine(char *test);


private:
    int fd;
    sockaddr_in sockAddr;
    
    bool alive;

    char readBuffer[READ_BUFFER_SIZE];
    int readIdx;
    int checkedIdx;
    int parsedIdx;



    char writeBuffer[WRITE_BUFFER_SIZE];

    METHOD method;
    char* url;
    char* version;
    char* host;

    std::atomic_bool isET;

};
std::atomic_int HttpConnection::totalConnection = 1;

