#include "HttpConnection.h"



void setNonblocking(int fd) {
    int cnt = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, cnt | O_NONBLOCK);
}

void addFd(int epollFd, int fd, bool isET) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;
    if (isET) {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    setNonblocking(fd);//位置待定
}

void removeFd(int epollFd, int fd) {
    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void resetFd(int epollFd, int fd, int option, bool isET) {
    epoll_event event;
    event.data.fd = fd;
    event.events = option | EPOLLRDHUP | EPOLLONESHOT;
    if (isET) {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
}

HttpConnection::HttpConnection():alive(false){
}

void HttpConnection::init(int epollFd_, int sockFD, const sockaddr_in & addr) {
    fd = sockFD;
    sockAddr = addr;
    epollFd = epollFd_;
    addFd(epollFd, fd, isET);
    alive = true;
    totalConnection++;
    LOG_INFO("Client %s:%d connect. Total connection : %d", 
             inet_ntoa(sockAddr.sin_addr), sockAddr.sin_port, totalConnection.load());
}


void HttpConnection::closeConnection() {
    alive = false;
    totalConnection--;
    removeFd(epollFd, fd);
    LOG_INFO("Client %s:%d quit. Total connection : %d", 
             inet_ntoa(sockAddr.sin_addr), sockAddr.sin_port, totalConnection.load());
}


HttpConnection::LINE_STATUS HttpConnection::parseLine(){
    char tmp;
    for (; checkedIdx < readIdx; checkedIdx++) {
        tmp = readBuffer[checkedIdx];
        if (tmp == '\r') {
            if (checkedIdx + 1 == readIdx) 
                return LINE_OPEN;
            if (readBuffer[checkedIdx + 1] == '\n') {
                readBuffer[checkedIdx++] = 0;
                readBuffer[checkedIdx++] = 0;
                return LINE_OK;
            }
            return LINE_BAD;
        }
        if (tmp == '\n')
            return LINE_BAD;
    }
    return LINE_OPEN;
}

bool HttpConnection::readToBuffer() {
    if (readIdx >= READ_BUFFER_SIZE) 
        return false;
    int len;
    do {
        len = recv(fd, readBuffer + readIdx, READ_BUFFER_SIZE - readIdx, 0);
        if (len <= 0) {
            if (len == -1 && isET && errno == EAGAIN) {
                return true;
            }
            return false;
        }
        readIdx += len;
    }while(isET);
    return true;
}

HttpConnection::HTTP_CODE HttpConnection::parseRequestLine(char* test) {
    url = strpbrk(test, " \t");    
    if (!url)
        return BAD_REQUEST;
    *url++ = '\0';
    char* tmp = test;
    if (strcasecmp(tmp, "GET"))
        method = GET;
    else if (strcasecmp(tmp, "POST")) 
        method = POST;
    else
        return BAD_REQUEST;
    url += strspn(url, " \t");

    version = strpbrk(url, " \t");
    if (!version)
        return BAD_REQUEST;
    *version++ = '\0';
    version += strspn(version, " \t");
    if (!strcasecmp(version, "HTTP/1.1"))
        return BAD_REQUEST;        

    
    if (strncasecmp(url, "http://", 7)) {
        url += 7;
        url = strchr(url, '/');
    }
    if (strncasecmp(url, "https://", 8)) {
        url += 8;
        url = strchr(url, '/');
    }

    if (!url)
        return BAD_REQUEST;

    return NO_REQUEST;
}
