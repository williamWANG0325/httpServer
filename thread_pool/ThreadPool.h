/*
 * @Author       : william
 * @Date         : 2022-02-11
*/
#pragma once

#include <thread>
#include <future>
#include <queue>
#include <vector>
#include <atomic>
#include <stdexcept>

class ThreadPool
{
public:
    explicit ThreadPool(int maxThread = 4);
    ~ThreadPool();
    template<class F, class... Args>
    auto append(F&& f, Args&& ... args) -> std::future<decltype(f(args ...))>;
    int freeThreadCount();
    int totalThreadCount();

private:
    void increse(int);

private:
    std::vector<std::thread> threadPool;
    std::mutex mutex_lock;
    std::condition_variable cv;
    std::queue<std::function<void()>> taskList;
    std::atomic<int> freeThread{0};
    std::atomic<bool> isWork{true};
};

ThreadPool::ThreadPool(int maxThread){
    increse(maxThread);
}

ThreadPool::~ThreadPool(){
    isWork = false;
    cv.notify_all();
    for(auto &i : threadPool)
        if(i.joinable()) {
            i.join();
        }
}

template<class F, class... Args>
auto ThreadPool::append(F&& f, Args&& ... args) -> std::future<decltype(f(args ...))>{
    if (!isWork) {
        throw std::runtime_error("append task to stopped thread pool.");
    }

    auto ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>> (std::bind(std::forward<F>(f), std::forward<Args>(args) ...));

    auto ans = ptr->get_future();

    {
        std::unique_lock<std::mutex> lock{mutex_lock};
        taskList.emplace([ptr]{(*ptr)();});
    }

    cv.notify_one();

    return ans;
}

void ThreadPool::increse(int size) {
    for (int i = 0; i < size; i++) {
        threadPool.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock{mutex_lock};

                    cv.wait(lock, [this] { return !isWork || !taskList.empty(); });

                    if (!isWork && taskList.empty())
                        return;
                    task = std::move(taskList.front());
                    taskList.pop();
                }
                --freeThread;
                task();
                ++freeThread;
            }
        });
        ++freeThread;
    }
}

int ThreadPool::freeThreadCount() {
    return freeThread;
}

int ThreadPool::totalThreadCount() {
    return threadPool.size();
}



