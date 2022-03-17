#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>


template<typename T> 
class BlockQueue
{
public:
    BlockQueue(int maxSize = 10000):maxSize_(maxSize), alive(true) {}
    ~BlockQueue() {}
    bool push(T&& task) {
        std::unique_lock<std::mutex> myLock{mutexLock};
        producerCV.wait(myLock, [&]{return taskQueue.size() < maxSize_ || !alive;});
        if(!alive)
            return false;
        taskQueue.emplace(std::forward<T>(task));
        consumerCV.notify_one();
        return true;
    }

    bool get(T& task) {
        std::unique_lock<std::mutex> myLock{mutexLock};
        consumerCV.wait(myLock, [&]{return taskQueue.size() > 0 || !alive;});
        if(!alive && taskQueue.empty())
            return false;
        task = taskQueue.front();
        taskQueue.pop();
        producerCV.notify_one();
        return true;
    }
    
    void destroyNow() {
        alive = false;
        consumerCV.notify_all();
        producerCV.notify_all();
    }

private:
    


private:
    std::queue<T> taskQueue;
    int maxSize_;
    std::mutex mutexLock;
    std::atomic<bool> alive;
    std::condition_variable producerCV;
    std::condition_variable consumerCV;
};

