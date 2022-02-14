/*
 * @Author       : william
 * @Date         : 2022-02-14
*/
#pragma once

#include "SQLConnection.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <chrono>

class SQLConnectionPool
{
public:
    ~SQLConnectionPool() ;
    void init(std::string ip_, unsigned int port_, std::string useName_, std::string password_, std::string dbName_, int maxConnection_, int minConnection_, int maxIdleTime_);
    SQLConnectionPool* getSQLPool();
    std::shared_ptr<SQLConnection> getConnection();

private:
    SQLConnectionPool();
    void produceConnection();
    void scanConnection();
    void resetPool();

private:
    std::atomic_bool alive;

    std::string ip;
    unsigned int port;
    std::string useName;
    std::string password;
    std::string dbName;

    int maxConnection;
    int minConnection;
    std::atomic_int totalConnection;

    std::queue<SQLConnection*> connectionQueue;
    std::mutex queueLock;

    std::thread producer;
    std::thread scanner;

    std::condition_variable producerCV;
    std::condition_variable consumerCV;
    std::condition_variable scannerCV;
    std::condition_variable retrieveAllCV;

    int maxIdleTime;

};

