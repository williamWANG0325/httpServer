#pragma once

#include <queue>
#include <chrono>
#include <functional>
#include <atomic>
#include <unordered_set>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <thread>

#include "../thread_pool/ThreadPool.h"

class Timer
{
public:
    struct TimerNode{
        std::chrono::time_point<std::chrono::steady_clock> timePoint;
        std::function<void()> callback;
        int nodeID;
        bool isPeriod;
        std::chrono::milliseconds period;
        bool operator<(const TimerNode& other) const {return timePoint > other.timePoint;}
    };

public:
    Timer();
    ~Timer();

    template <typename T, typename... Args>
    int append(int waitTime, bool isPeriod, T&& fun, Args&&... args);


private:
    void run();


private:
    std::priority_queue<TimerNode> heap; 
    std::mutex heapLock;
    

    std::unordered_set<int> aliveSet;
    std::mutex setLock;


    std::atomic_bool isAlive;
    std::atomic_int id;
    std::condition_variable cv;

    std::thread childThread;
    ThreadPool threadPool;
};

Timer::Timer():isAlive(true), id(0), childThread(std::thread([this]{run();})) {}

Timer::~Timer(){
    isAlive = false;
    cv.notify_all();
    if (childThread.joinable())
        childThread.join();
}

template <typename T, typename... Args>
int Timer::append(int waitTime, bool isPeriod, T&& fun, Args&&... args){
    if (!isAlive)
        return -1;
    TimerNode timerNode;
    timerNode.nodeID = id.fetch_add(1);
    if (timerNode.nodeID == -1) // -1 is for error
        timerNode.nodeID = id.fetch_add(1);
    {
        std::unique_lock<std::mutex> ulock{setLock};
        aliveSet.emplace(timerNode.nodeID);
    }
    timerNode.isPeriod = isPeriod;
    timerNode.period = std::chrono::milliseconds(waitTime);
    timerNode.timePoint = std::chrono::steady_clock::now() + timerNode.period;

    //timerNode.callback = [&]{bind(std::forward<T>(fun), std::forward<Args>(args)...)();};
    
    timerNode.callback = bind(std::forward<T>(fun), std::forward<Args>(args)...);

    {
        std::unique_lock<std::mutex> ulock{heapLock};
        heap.push(timerNode);
    }
    cv.notify_all();
    return timerNode.nodeID; 
}

void Timer::run(){
    while (true) {
        std::unique_lock<std::mutex> ulock{heapLock};
        cv.wait(ulock, [&]{return !heap.empty() || !isAlive;});
        if (heap.empty() && !isAlive)
            return;
        auto tmp = heap.top();
        auto diff = tmp.timePoint - std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() > 0) {
            cv.wait_for(ulock, diff);
            continue;
        } else {
            heap.pop();
            if(!aliveSet.count(tmp.nodeID))
                continue;
            if(tmp.isPeriod){
                tmp.timePoint = std::chrono::steady_clock::now() + tmp.period;
                heap.emplace(tmp);
            }
            threadPool.append(tmp.callback);
        }
    }
}
