#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include "logger.h"

class ThreadPool {
private:
    std::vector<std::thread> mthreads;
    std::queue<std::function<void(void)>> mtasks;
    std::mutex mmutexQueue;
    std::condition_variable mcv;
    bool mstop;
    int mmaxThread = std::thread::hardware_concurrency();

public:
    ThreadPool(int number_thread);
    ~ThreadPool();
    void enqueue(std::function<void(void)>);

};

ThreadPool::ThreadPool(int number_thread) : mmaxThread(number_thread) {
    mstop = false;
    for(int i = 0; i < mmaxThread; i++) {
        mthreads.emplace_back([&]() {
            while (true)
            {
                std::function<void(void)> task;

                {
                    std::unique_lock<std::mutex> lock(mmutexQueue);
                    mcv.wait(lock, [this]() {
                        return !mtasks.empty() || mstop;
                    });

                    if(mstop) {
                        return; 
                    }

                    task = std::move(mtasks.front());
                    mtasks.pop();
                }
                DEBUG() << "[ThreadPool] " << "Task started";
                task();
                DEBUG() << "[ThreadPool] " << "Task done";
            }
            
        });
    }
}

ThreadPool::~ThreadPool() {
    DEBUG() << "[ThreadPool] " << "Destructor. Every tasks should be terminate.";
    {
        std::unique_lock<std::mutex> lock(mmutexQueue);
        mstop = true;
    }

    mcv.notify_all();

    for(auto& thread : mthreads) {
        if(thread.joinable()) thread.join();
    }

}

void ThreadPool::enqueue(std::function<void(void)> task) {
    {
        std::lock_guard<std::mutex> lock(mmutexQueue);
        mtasks.emplace(move(task));
    }
    mcv.notify_one();
}

#endif // __THREAD_POOL__
