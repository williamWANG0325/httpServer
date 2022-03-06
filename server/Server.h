#pragma once

#include <sys/epoll.h>
#include <unordered_map>

#include "Epoll.h"
#include "../log_system/Log.h"
#include "../thread_pool/ThreadPool.h"
#include "../http_connection/HttpConnection.h"


class Server
{
public:
    Server(int port, bool isListenET = true, bool isConnET = true);
    ~Server();
    void mainLoop();


private:
    bool initSocket();
    
    void connAccept();
    void connRead(int fd);
    void connWrite(int fd);

    void setFdNonBlock(int fd);

private:
    bool isAlive;

    Epoll epoller;

    ThreadPool threadPool;

    int listenFd;
    int servPort;
    
    uint32_t listenEvent;
    uint32_t connEvent;

    std::unordered_map<int, HttpConnection> connMap;


};

