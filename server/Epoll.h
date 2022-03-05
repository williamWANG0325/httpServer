#pragma once


#include <sys/epoll.h>
#include <assert.h>
#include <memory>
#include <unistd.h>

class Epoll
{
public:
    Epoll(int maxEvents = 1024);
    
    ~Epoll();

    bool addFd(int fd, uint32_t events);

    bool modFd(int fd, uint32_t events);

    bool delFd(int fd);

    int wait(int timeoutMs = -1);

    int getEventFd(size_t i);

    uint32_t getEvents(size_t i);

private:
    int epollFd_;
    int maxEvents_;
    epoll_event* events_;
};
