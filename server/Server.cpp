#include "Server.h"


Server::Server(int port, bool isListenET, bool isConnET): servPort(port){
    // ...
    
    isAlive = true;

    listenEvent = EPOLLRDHUP;
    if (isListenET)
        listenEvent |= EPOLLET;

    connEvent = EPOLLRDHUP | EPOLLONESHOT;
    if (isConnET){
        connEvent |= EPOLLET;
        HttpConnection::isET = true;
    }
    initSocket();
}

Server::~Server(){
    isAlive = false;
    close(listenFd);
    for(auto i : connMap)
        i.second.closeConnection();
    // ...
}


void Server::setFdNonBlock(int fd){
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD) | O_NONBLOCK);    // set fd no block
}

bool Server::initSocket() {
    sockaddr_in servAddr;

    listenFd = socket(PF_INET, SOCK_STREAM, 0);
    setFdNonBlock(listenFd);

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);   //INADDR_ANY is 0.0.0.0
    servAddr.sin_port = htons(servPort);
    bind(listenFd, (sockaddr *) &servAddr, sizeof(servAddr));

    listen(listenFd, 128);

    epoller.addFd(listenFd, EPOLLIN);

    return true;
}



void Server::mainLoop() {
    while(isAlive) {
        int evectCnt = epoller.wait();
        for (int i = 0;i < evectCnt; ++i) {
            int fd = epoller.getEventFd(i);
            if (fd == listenFd) {
                connAccept(); // accept connection
                continue;
            }
            uint32_t event = epoller.getEvents(i);
            if (event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                epoller.delFd(fd);
                connMap[fd].closeConnection();
            }else if (event & EPOLLIN){
                threadPool.append([this, fd]{this->connRead(fd);});
            }else if (event & EPOLLOUT){
                threadPool.append([this, fd]{this->connWrite(fd);});
            }else {
                LOG_ERROR("Unexpexted Event");
            }
        }

    }
}

void Server::connAccept() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd, (struct sockaddr *)&addr, &len);
        if(fd <= 0)
            return;
        setFdNonBlock(fd);
        connMap[fd].init(fd, addr);
        epoller.addFd(fd, connEvent | EPOLLIN);
    } while(listenEvent & EPOLLET);
}

void Server::connRead(int fd){
    HttpConnection::HTTP_CODE tmp = connMap[fd].processRead();
    if (tmp == HttpConnection::NO_REQUEST)
        epoller.modFd(fd, connEvent | EPOLLIN);
    if (tmp == HttpConnection::GET_REQUEST)
        epoller.modFd(fd, connEvent | EPOLLOUT);
}

void Server::connWrite(int fd){
    HttpConnection::HTTP_CODE tmp = connMap[fd].processWrite();
    if (tmp == HttpConnection::INCOMPLETE_WRITE)
        epoller.modFd(fd, connEvent | EPOLLOUT);
    if (tmp == HttpConnection::COMPLETE_WRITE){
        //close;
    }
    if (tmp == HttpConnection::KEEP_ALIVE)
        epoller.modFd(fd, connEvent | EPOLLIN);
}

