#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <queue>
#include <vector>

class ThreadPool {
    public:

    ThreadPool(int max_threads) : shutdown_signal(false) {
        // Spin up worker threads.
        this->threads.reserve(max_threads);
        for (int i = 0; i < max_threads; i++) {
            this->threads.emplace_back(
                std::bind(&ThreadPool::worker, this, i));
        }
    }

    ~ThreadPool() {
        if (!this->shutdown_signal) {
            this->stop();
        }
    }

    void stop() {
        // Unblock all threads and signal to stop.
        std::unique_lock<std::mutex> guard (this->lock);
        this->shutdown_signal = true;
        this->resume.notify_all();
        guard.unlock();

        // Wait for all threads to terminate.
        for (auto & thread : this->threads) {
            thread.join();
        }

    }

    template<typename F, typename... Args>
    auto add_task(F function, Args... args) -> std::future<decltype(function(args...))> {
        std::unique_lock<std::mutex> guard (this->lock);

        using ReturnType = decltype(function(args...));

        typedef std::pair<std::promise<ReturnType>, std::function<ReturnType()>>
            promise_task_pair;

        std::function<ReturnType()> task = std::bind(function,
            std::forward<Args>(args)...);

        std::shared_ptr<promise_task_pair> promise_task =
            std::make_shared<promise_task_pair>(
                std::promise<ReturnType>(), std::move(task));

        std::future<ReturnType> future = promise_task->first.get_future();

        // Queue a task and wake a thread.
        this->tasks.push([promise_task]() {
            // Attempt to fullfill the task's associated promise with the value
            // returned from the task.
            try {
                promise_task->first.set_value(promise_task->second());
            } catch (...) {
                promise_task->first.set_exception(std::current_exception());
            }
        });
        this->resume.notify_one();

        return std::move(future);
    }

    private:

    std::mutex lock;
    std::condition_variable resume;
    bool shutdown_signal;
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;

    void worker(const int i) {
        std::function<void(void)> task;

        while (1) {
            {
                std::unique_lock<std::mutex> guard (this->lock);

                while (!this->shutdown_signal && this->tasks.empty()) {
                    this->resume.wait(guard);
                }

                // Terminate thread if signaled to stop and no tasks remain.
                if (this->tasks.empty()) {
                    return;
                }

                // Pop task from task queue.
                task = this->tasks.front();
                this->tasks.pop();
            }

            // Run task without locking.
            task();
        }
    }
};
#endif
