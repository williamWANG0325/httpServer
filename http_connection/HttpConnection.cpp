#include "HttpConnection.h"


std::atomic_int HttpConnection::totalConnection{0};
char HttpConnection::rootPath[100] = ".";
std::atomic_bool HttpConnection::isET{false};



HttpConnection::HttpConnection():alive(false){
}

void HttpConnection::initStatic(const char* root, bool ET) {
    totalConnection = 0;
    strcpy(rootPath, root);
    isET = ET;
    
}

void HttpConnection::init(int sockFD, const sockaddr_in & addr) {
    fd = sockFD;
    sockAddr = addr;
    alive = true;
    totalConnection++;
    LOG_INFO("Client %s:%d connect. Total connection : %d", 
             inet_ntoa(sockAddr.sin_addr), sockAddr.sin_port, totalConnection.load());
    init();
}
void HttpConnection::init(){
    //  清空所有数据
    writeIdx = 0;
    readIdx = 0;
    
}
void HttpConnection::closeConnection() {
    alive = false;
    totalConnection--;
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

HttpConnection::HTTP_CODE HttpConnection::parseRequestLine(char* text) {
    url = strpbrk(text, " \t");    
    if (!url)
        return BAD_REQUEST;
    *url++ = '\0';
    char* tmp = text;
    if (!strcasecmp(tmp, "GET"))
        method = GET;
    else if (!strcasecmp(tmp, "POST")) 
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

    
    if (!strncasecmp(url, "http://", 7)) {
        url += 7;
        url = strchr(url, '/');
    }
    if (!strncasecmp(url, "https://", 8)) {
        url += 8;
        url = strchr(url, '/');
    }

    if (!url)
        return BAD_REQUEST;

    masterState = CHECK_STATE_HEADER;

    return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::parseHeaders(char* text) {
    if (text[0] == '\0'){
        if (method == GET)
            return GET_REQUEST;
        masterState = CHECK_STATE_CONTENT;
        return NO_REQUEST;
    }

    if (!strncasecmp(text, "Connection:", 11))
    {
        text += 11;
        text += strspn(text, " \t");
        if (!strcasecmp(text, "keep-alive"))
        {
            keepAlive = true;
        }
    }
    else if (!strncasecmp(text, "Content-length:", 15))
    {
        text += 15;
        text += strspn(text, " \t");
        contentLen = atol(text);
    }
    else if (!strncasecmp(text, "Host:", 5))
    {
        text += 5;
        text += strspn(text, " \t");
        host = text;
    }
    else
    {
        LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::parseContent(char* text) {
    if (readIdx >= checkedIdx + contentLen) {
        text[contentLen] = '\0';
        content = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}


HttpConnection::HTTP_CODE HttpConnection::parse() {
    LINE_STATUS lineStatus = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = nullptr;

    while ((masterState != CHECK_STATE_CONTENT) && ((lineStatus = parseLine()) == LINE_OK)) {
        text = readBuffer + parsedIdx;
        parsedIdx = checkedIdx;
        switch (masterState) {
            case CHECK_STATE_REQUESTLINE:{
                ret = parseRequestLine(text);
                if (ret == BAD_REQUEST)
                    return ret;
                break;
            }
            case CHECK_STATE_HEADER:{
                ret = parseHeaders(text);
                if (ret == BAD_REQUEST)
                    return ret;
                if (ret == GET_REQUEST) {
                    HTTP_CODE tmp = doRequest();
                    fillWriteBuffer(tmp);
                    return GET_REQUEST;
                }
                break;                  
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    if (masterState == CHECK_STATE_CONTENT) {
        ret = parseContent(readBuffer + parsedIdx);
        if (ret == GET_REQUEST){
            HTTP_CODE tmp = doRequest();
            fillWriteBuffer(tmp);
            return GET_REQUEST;
        }else
            return NO_REQUEST;
    }
    if (lineStatus == LINE_BAD)
        return BAD_REQUEST;
    else
        return NO_REQUEST;
}

bool HttpConnection::addResponse(const char* format, ...) {
    if (writeIdx >= WRITE_BUFFER_SIZE - 1)
        return false;
    va_list tmp;
    va_start(tmp, format);
    int len = vsnprintf(writeBuffer + writeIdx, WRITE_BUFFER_SIZE - 1 - writeIdx, format, tmp);
    writeIdx += len;
    va_end(tmp);
    return true;
}
bool HttpConnection::addStatusLine(int status, const char* title){
    return addResponse("%s %d %s\r\n","HTTP/1.1",status,title);
}
bool HttpConnection::addContentType(char* s) {
    char tmp[20] = "text/html";
    if (s == nullptr)
        s = tmp;
    return addResponse("Content-Type:%s\r\n", s);
}
bool HttpConnection::addHeaders(int responseLen){
    bool flag = true;
    flag = flag && addResponse("Content-Length:%d\r\n", responseLen);
    flag = flag && addResponse("Connection:%s\r\n", (keepAlive == true) ? "keep-alive" : "close");
    flag = flag && addResponse("\r\n");
    return flag;
}

HttpConnection::HTTP_CODE HttpConnection::doRequest() {
    strcpy(requestFile, rootPath);
    int len = strlen(requestFile);
    const char* ptr = strchr(url, '/');
    if (method == POST){

    }
    strcpy(requestFile + len, ptr);
    if (stat(requestFile, &fileStat))
        return NO_RESOURCE;
    if (!(fileStat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;
    if (S_ISDIR(fileStat.st_mode))
        return BAD_REQUEST;

    int tmpFd = open(requestFile, O_RDONLY);
    mmapWriteFile = (char *) mmap(0, fileStat.st_size, PROT_READ, MAP_PRIVATE, tmpFd, 0);
    close(tmpFd);

    return FILE_REQUEST;
}

void HttpConnection::unmap() {
    if (mmapWriteFile) {
        munmap(mmapWriteFile, fileStat.st_size);
    }
}

bool HttpConnection::fillWriteBuffer(HttpConnection::HTTP_CODE ret) {
    switch(ret) 
    {
    case FILE_REQUEST:
        {
            addStatusLine(200, "OK");
            if (fileStat.st_size != 0) {
                addHeaders(fileStat.st_size);

                writeIV[0].iov_base = writeBuffer;
                writeIV[0].iov_len = writeIdx;

                writeIV[1].iov_base = mmapWriteFile;
                writeIV[1].iov_len = fileStat.st_size;

                bytesToSend = writeIdx + fileStat.st_size;
                ivCount = 2;
                return true;

            }
        }
    default:
        return false;
    }
    writeIV[0].iov_base = writeBuffer;
    writeIV[0].iov_len = writeIdx;
    ivCount = 1;
    return true;

}

HttpConnection::HTTP_CODE HttpConnection::processWrite() {
    int tmp = 0;
    while (true) {
        tmp = writev(fd, writeIV, ivCount);
        if (tmp > 0) 
            bytesHaveSend +=tmp;
        else {
            if (errno == EAGAIN) {
                if (bytesHaveSend >= (int)writeIV[0].iov_len) {
                    writeIV[0].iov_len  = 0;
                    writeIV[1].iov_base = mmapWriteFile + (bytesHaveSend - writeIdx);
                    writeIV[1].iov_len = bytesToSend;
                } else {
                    writeIV[0].iov_base = writeBuffer + bytesHaveSend;
                    writeIV[0].iov_len = writeIdx - bytesHaveSend;
                }
   //             resetFd(epollFd, fd, EPOLLOUT, isET);
                return INCOMPLETE_WRITE;
            }
            unmap();
            return INTERNAL_ERROR;
        }
        bytesToSend -= tmp;
        if (bytesToSend <= 0) {
            unmap();
            if (keepAlive) {
                init();
   //             resetFd(epollFd, fd, EPOLLIN, isET);
                return KEEP_ALIVE;
            }
            return COMPLETE_WRITE;
        }
    }
}


HttpConnection::HTTP_CODE HttpConnection::processRead(){
    readToBuffer();
    return parse();
}

