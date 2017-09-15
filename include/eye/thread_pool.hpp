#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <queue>
#include <utility>
#include <vector>

namespace eye {
    class ThreadPool {
        public:

        void stop();
        template<typename F, typename... Args>
        auto queue_task(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;
        ThreadPool(const std::size_t max_threads);
        ~ThreadPool();

        private:

        std::mutex queue_mutex;
        std::condition_variable resume;
        bool shutdown;
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;

        void worker();
    };

    inline void ThreadPool::stop() {
        {
            // Signal all threads to wrap up.
            std::lock_guard<std::mutex> lock(this->queue_mutex);
            this->shutdown = true;
            this->resume.notify_all();
        }

        // Wait for all threads to terminate.
        for (auto & thread : this->workers) {
            thread.join();
        }
    }

    template<typename F, typename... Args>
    inline auto ThreadPool::queue_task(F&& f, Args&&... args)
            -> std::future<decltype(f(args...))> {
        {
            std::lock_guard<std::mutex> lock(this->queue_mutex);
            if (this->shutdown) {
                std::runtime_error(
                    "Cannot queue task after thread pool has shut down.");
            }
        }

        using ReturnType = decltype(f(args...));

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<ReturnType> result = task->get_future();

        {
            std::lock_guard<std::mutex> lock(this->queue_mutex);
            this->tasks.emplace([task]() { (*task)(); });
        }

        this->resume.notify_one();

        return result;
    }

    inline ThreadPool::ThreadPool(const std::size_t max_threads) : shutdown(false) {
        // Spin up worker threads.
        this->workers.reserve(max_threads);
        for (std::size_t i = 0; i < max_threads; i++) {
            this->workers.emplace_back(
                std::bind(&ThreadPool::worker, this));
        }
    }

    inline ThreadPool::~ThreadPool() {
        if (!this->shutdown) {
            this->stop();
        }
    }

    inline void ThreadPool::worker() {
        std::function<void(void)> task;

        while (1) {
            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);

                while (!this->shutdown && this->tasks.empty()) {
                    this->resume.wait(lock);
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
}
#endif
