#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <queue>
#include <vector>

namespace eye {
    template <typename ReturnType>
    struct PromiseTask {
        typedef std::promise<ReturnType> promise_t;
        typedef std::function<ReturnType()> function_t;

        promise_t promise;
        function_t task;

        PromiseTask(promise_t p, function_t t) {
            this->promise = std::move(p);
            this->task = std::move(t);
        }
    };

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

            // Determine return type of task.
            using ReturnType = decltype(function(args...));

            // Associate a promise to capture the return value of a task.
            using PairType = PromiseTask<ReturnType>;

            std::function<ReturnType()> task = std::bind(function,
                std::forward<Args>(args)...);

            std::shared_ptr<PairType> task_data = std::make_shared<PairType>(
                PairType(std::promise<ReturnType>(), task));

            std::future<ReturnType> future = task_data->promise.get_future();

            // Queue a task and wake a thread.
            this->tasks.push([this, task_data] {
                // Needs to be moved because it holds a promise.
                this->run_task<ReturnType>(std::move(task_data));
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

        template<typename ReturnType>
        void run_task(std::shared_ptr<PromiseTask<ReturnType>> task_data) {
            // Attempt to fullfill the task's associated promise with the value
            // returned from the task.
            try {
                task_data->promise.set_value(task_data->task());
            } catch (...) {
                task_data->promise.set_exception(std::current_exception());
            }
        }
    };

    // Template specialization for tasks with void return types.
    template<>
    void inline ThreadPool::run_task<void>(std::shared_ptr<PromiseTask<void>> task_data) {
        try {
            task_data->task();
            task_data->promise.set_value();
        } catch (...) {
            task_data->promise.set_exception(std::current_exception());
        }
    }
}
#endif
