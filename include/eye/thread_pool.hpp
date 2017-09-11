#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/future.hpp>

namespace eye {
    template <typename ReturnType>
    struct PromiseTask {
        using promise_t = boost::promise<ReturnType>;
        using function_t = boost::function<ReturnType()>;

        promise_t promise;
        function_t task;

        PromiseTask(promise_t p, function_t t) {
            this->promise = boost::move(p);
            this->task = boost::move(t);
        }
    };

    class ThreadPool {
        public:

        int task_counter;

        ThreadPool(int max_threads) : shutdown_signal(false) {
            this->task_counter = 0;

            // Spin up worker threads.
            this->threads.reserve(max_threads);
            for (int i = 0; i < max_threads; i++) {
                this->threads.emplace_back(
                    boost::bind(&ThreadPool::worker, this, i));
            }
        }

        ~ThreadPool() {
            if (!this->shutdown_signal) {
                this->stop();
            }
        }

        void stop() {
            // Unblock all threads and signal to stop.
            boost::unique_lock<boost::mutex> guard (this->lock);
            this->shutdown_signal = true;
            this->resume.notify_all();
            guard.unlock();

            // Wait for all threads to terminate.
            for (auto & thread : this->threads) {
                thread.join();
            }

            std::cout << this->task_counter << " tasks executed." << std::endl;
        }

        template<typename F, typename... Args>
        auto add_task(F function, Args... args) -> boost::future<decltype(function(args...))> {
            boost::unique_lock<boost::mutex> guard (this->lock);

            // Determine return type of task.
            using ReturnType = decltype(function(args...));

            // Associate a promise to capture the return value of a task.
            using PairType = PromiseTask<ReturnType>;

            boost::function<ReturnType()> task = boost::bind(function,
                boost::forward<Args>(args)...);

            boost::shared_ptr<PairType> task_data = boost::make_shared<PairType>(
                PairType(boost::promise<ReturnType>(), task));

            boost::future<ReturnType> future = task_data->promise.get_future();

            // Queue a task and wake a thread.
            this->tasks.push([this, task_data] {
                // Needs to be moved because it holds a promise.
                this->run_task<ReturnType>(boost::move(task_data));
            });
            this->resume.notify_one();

            return boost::move(future);
        }

        private:

        boost::mutex lock;
        boost::condition_variable resume;
        bool shutdown_signal;
        std::vector<boost::thread> threads;
        std::queue<boost::function<void()>> tasks;

        void worker(const int i) {
            boost::function<void(void)> task;

            while (1) {
                {
                    boost::unique_lock<boost::mutex> guard (this->lock);

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
                    this->task_counter++;
                }

                // Run task without locking.
                task();
            }
        }

        template<typename ReturnType>
        void run_task(boost::shared_ptr<PromiseTask<ReturnType>> task_data) {
            // Attempt to fullfill the task's associated promise with the value
            // returned from the task.
            try {
                task_data->promise.set_value(task_data->task());
            } catch (...) {
                task_data->promise.set_exception(boost::current_exception());
            }
        }
    };

    // Template specialization for tasks with void return types.
    template<>
    void inline ThreadPool::run_task<void>(boost::shared_ptr<PromiseTask<void>> task_data) {
        try {
            task_data->task();
            task_data->promise.set_value();
        } catch (...) {
            task_data->promise.set_exception(boost::current_exception());
        }
    }
}
#endif
