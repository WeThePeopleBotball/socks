#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

/**
 * @file ThreadPool.hpp
 * @brief A minimal yet powerful thread pool implementation for task
 * parallelization.
 *
 * Provides enqueuing of regular tasks and submitting of asynchronous tasks with
 * futures. Supports graceful shutdown (wait for tasks) and immediate shutdown
 * (terminate without waiting).
 */

namespace Socks {

/**
 * @class ThreadPool
 * @brief A thread pool that manages a fixed number of worker threads to execute
 * tasks concurrently.
 *
 * Features:
 * - Safe task submission with automatic termination checking.
 * - Graceful and immediate shutdown.
 * - Asynchronous task submission returning std::future.
 */
class ThreadPool {
  public:
    /**
     * @brief Constructs a ThreadPool with the specified number of worker
     * threads.
     * @param thread_count Number of worker threads to create.
     */
    explicit ThreadPool(size_t thread_count);

    /**
     * @brief Destroys the ThreadPool, waiting for tasks to complete (graceful
     * shutdown).
     */
    ~ThreadPool();

    /**
     * @brief Submits a function asynchronously to the pool and returns a
     * std::future.
     *
     * @tparam F Function type.
     * @tparam Args Argument types.
     * @param f Callable object (function, lambda, etc.).
     * @param args Arguments to pass to the callable.
     * @return std::future<ReturnType> Future associated with the callable's
     * return value.
     */
    template <typename F, typename... Args>
    auto submit_async(F &&f, Args &&...args)
        -> std::future<decltype(f(args...))>;

    /**
     * @brief Enqueues a fire-and-forget task to the pool.
     * @param task Callable task without return value.
     *
     * @throws std::runtime_error if the pool is stopping or terminating.
     */
    void enqueue(std::function<void()> task);

    /**
     * @brief Waits for all currently enqueued tasks to complete and joins all
     * threads.
     *
     * After calling wait(), the thread pool cannot be reused.
     */
    void wait();

    /**
     * @brief Immediately signals all worker threads to exit without completing
     * pending tasks.
     *
     * Running tasks will finish if they do not manually check
     * should_terminate().
     */
    void terminate();

    /**
     * @brief Checks if termination has been requested.
     *
     * Tasks may use this method to voluntarily abort early.
     * @return true if terminate() has been called.
     */
    bool should_terminate() const { return terminate_now_; }

  private:
    /**
     * @brief Main loop executed by each worker thread.
     *
     * Fetches tasks from the queue and executes them, checking for termination
     * requests.
     */
    void worker();

    /**
     * @brief Internal helper to wrap tasks and automatically check for
     * termination.
     *
     * If termination is requested, the task will not run.
     *
     * @tparam Func Task function type.
     * @param f The task to wrap.
     * @return A wrapped version of the task.
     */
    template <typename Func> std::function<void()> wrap_task(Func &&f);

    std::vector<std::thread> workers_;        ///< Vector of worker threads
    std::queue<std::function<void()>> tasks_; ///< Queue of pending tasks

    std::mutex queue_mutex_; ///< Protects access to the task queue
    std::condition_variable
        condition_;          ///< Signals when new tasks are available
    std::atomic<bool> stop_; ///< Indicates graceful stop requested
    std::atomic<bool>
        terminate_now_; ///< Indicates immediate termination requested
};

// ------------ Template Implementations -------------

template <typename Func> std::function<void()> ThreadPool::wrap_task(Func &&f) {
    return [this, func = std::forward<Func>(f)]() {
        if (should_terminate())
            return;
        try {
            func();
        } catch (...) {
            // Optional: handle/log exception internally
        }
    };
}

template <typename F, typename... Args>
auto ThreadPool::submit_async(F &&f, Args &&...args)
    -> std::future<decltype(f(args...))> {
    using ReturnType = decltype(f(args...));

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<ReturnType> res = task->get_future();
    enqueue(wrap_task([task]() { (*task)(); }));
    return res;
}

} // namespace Socks
