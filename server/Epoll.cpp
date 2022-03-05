#include "Epoll.h"



Epoll::Epoll(int maxEvents):epollFd_(epoll_create(1)), maxEvents_(maxEvents) {
    assert(maxEvents_ > 0 && epollFd_ >= 0);
    events_ = new epoll_event[maxEvents_];
}

Epoll::~Epoll() {
    close(epollFd_);
    delete []events_;
}

bool Epoll::addFd(int fd, uint32_t events) {
    if (fd < 0)
        return false;
    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    return !epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event);
}

bool Epoll::modFd(int fd, uint32_t events) {
    if (fd < 0)
        return false;
    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    return !epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event);
}

bool Epoll::delFd(int fd) {
    if (fd < 0)
        return false;
    return !epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
}

int Epoll::wait(int timeoutMs) {
    return epoll_wait(epollFd_, events_, maxEvents_, timeoutMs);
}

int Epoll::getEventFd(size_t i) {
    return events_[i].data.fd;
}

uint32_t Epoll::getEvents(size_t i) {
    return events_[i].events;
}
