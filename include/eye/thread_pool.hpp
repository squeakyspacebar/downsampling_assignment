#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP
#define BOOST_THREAD_PROVIDES_FUTURE

#include <condition_variable>
#include <functional>
#include <memory>
#include <thread>
#include <queue>
#include <utility>
#include <vector>
#include <boost/thread/future.hpp>

namespace eye {
    /**
     * A pair data type to link promises to their corresponding tasks.
     * Created to allow return type deduction for task-running logic.
     */
    template <typename ReturnType>
    struct PromiseTask {
        using promise_t = boost::promise<ReturnType>;
        using function_t = std::function<ReturnType()>;

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
        auto add_task(F function, Args... args) -> boost::future<decltype(function(args...))> {
            std::unique_lock<std::mutex> guard (this->lock);

            // Determine return type of function.
            using ReturnType = decltype(function(args...));

            // Associate a promise to capture the return value of a task.
            using PairType = PromiseTask<ReturnType>;

            std::function<ReturnType()> task = std::bind(function,
                std::forward<Args>(args)...);

            std::shared_ptr<PairType> task_data = std::make_shared<PairType>(
                PairType(boost::promise<ReturnType>(), task));

            boost::future<ReturnType> future = task_data->promise.get_future();

            // Queue a task and wake a thread.
            this->tasks.push([this, task_data] {
                // task_data needs to be moved because it contains a promise.
                this->run_task(std::move(task_data));
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
                { // Mutex-locked scope.
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

    // Template specialization for tasks with void return types, since we can't
    // set the promise to the tasks's return value.
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
