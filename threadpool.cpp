#include "threadpool.hpp"

#include <iostream>

namespace Socks {

ThreadPool::ThreadPool(size_t thread_count)
    : stop_(false), terminate_now_(false) {
    std::cout << "\033[1;32m[ThreadPool] INFO: Starting thread pool with "
              << thread_count << " threads.\033[0m" << std::endl;
    for (size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() { wait(); }

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] {
                return terminate_now_ || (!tasks_.empty() && !stop_);
            });

            if (terminate_now_) {
                std::cout
                    << "\033[1;31m[ThreadPool] INFO: Worker thread exiting "
                       "immediately due to termination request.\033[0m"
                    << std::endl;
                return;
            }

            if (stop_ && tasks_.empty()) {
                return;
            }

            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            } else {
                continue;
            }
        }

        task(); // already wrapped with auto-abort safety
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_ || terminate_now_) {
            throw std::runtime_error(
                "[ThreadPool] Cannot enqueue on stopped or terminated pool.");
        }
        tasks_.emplace(wrap_task(std::move(task)));
    }
    condition_.notify_one();
}

void ThreadPool::wait() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (std::thread &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    std::cout << "\033[1;32m[ThreadPool] INFO: All worker threads joined "
                 "successfully.\033[0m"
              << std::endl;
}

void ThreadPool::terminate() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        terminate_now_ = true;
    }
    condition_.notify_all();
    for (std::thread &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    std::cout << "\033[1;31m[ThreadPool] WARNING: Thread pool terminated "
                 "immediately. Pending tasks may have been abandoned.\033[0m"
              << std::endl;
}

} // namespace Socks
