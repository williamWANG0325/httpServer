/*
 * @Author       : william
 * @Date         : 2022-02-14
*/
#include "SQLConnectionPool.h"

void SQLConnectionPool::init(std::string ip_, unsigned int port_, std::string useName_, std::string password_, std::string dbName_, int maxConnection_ = 10, int minConnection_ = 4, int maxIdleTime_ = -1)
{

    ip = ip_;
	port = port_;
	useName = useName_;
	password = password_;
	dbName = dbName_;
	maxConnection = maxConnection_;
	minConnection = minConnection_;
	totalConnection = 0;
    maxIdleTime = maxIdleTime_;
    alive = true;

    for (int i = 0; i < minConnection; ++i) {
        SQLConnection* connection = new SQLConnection();
        connection->connect(ip, port, useName, password, dbName);
        connectionQueue.emplace(connection);
        totalConnection++;
    }


    producer = std::thread(std::bind(&SQLConnectionPool::produceConnection, this));

    if(maxIdleTime > 0)
        scanner = std::thread(std::bind(&SQLConnectionPool::scanConnection, this)); 

}

SQLConnectionPool* SQLConnectionPool::getSQLPool(){
    static SQLConnectionPool SQLPool;
    return &SQLPool;
}

std::shared_ptr<SQLConnection> SQLConnectionPool::getConnection(){
        
    std::shared_ptr<SQLConnection> sptr;
    if (!alive) {
        // 连接池未初始化
        return sptr;
    }

    std::unique_lock<std::mutex> mylock(queueLock);
    producerCV.wait(mylock, [this]{return !connectionQueue.empty() || !alive;});

    if (!alive) {
        // 连接池已关闭
        return sptr;
    }
    sptr.reset(connectionQueue.front(), [&](SQLConnection* tmp){
                std::unique_lock<std::mutex> mylock(queueLock);
                tmp->refreshAliveTime();
                connectionQueue.emplace(tmp);
                consumerCV.notify_one();
               });
    connectionQueue.pop();
    if (totalConnection < maxConnection) {
        producerCV.notify_one();
    }
    return sptr;    
}

void SQLConnectionPool::produceConnection() {
    while (true) {
        std::unique_lock<std::mutex> mylock(queueLock);
        consumerCV.wait(mylock, [this]{return connectionQueue.empty() || !alive;});
        if (!alive) {
            return;
        }
        if (totalConnection < maxConnection) {
            SQLConnection* connection = new SQLConnection();
            connection->connect(ip, port, useName, password, dbName);
            connectionQueue.emplace(connection);
            totalConnection++;
        }
        consumerCV.notify_one(); 
    }
}

void SQLConnectionPool::scanConnection() {
    std::mutex mu;
    std::unique_lock<std::mutex> tmplock{mu};
    while(scannerCV.wait_for(tmplock, std::chrono::seconds(maxIdleTime), [this]{return !alive;}) == false)
    {
       std::unique_lock<std::mutex>  mylock{queueLock};
       while (totalConnection > minConnection) {
           SQLConnection* ptr = connectionQueue.front();
           if (ptr->getAliveTime() > maxIdleTime * 1000) {
               connectionQueue.pop();
               --totalConnection;
               delete ptr;
           } else {
               break;
           }
       }
    }
}

void SQLConnectionPool::resetPool() {
    if(!alive)
        return;
    alive = false;

    producerCV.notify_all();
    consumerCV.notify_all();
    scannerCV.notify_all();
    
    if (producer.joinable())
        producer.join();
    if (scanner.joinable())
        scanner.join();

    std::unique_lock<std::mutex> mylock(queueLock);
    retrieveAllCV.wait(mylock, [&]{return (int)connectionQueue.size() >= totalConnection;});
    while (!connectionQueue.empty()) {
        auto ptr = connectionQueue.front();
        connectionQueue.pop();
        delete ptr;
    }

    
}

SQLConnectionPool::SQLConnectionPool():alive(false) {
}


SQLConnectionPool::~SQLConnectionPool() {
    resetPool();
    mysql_library_end();
}

