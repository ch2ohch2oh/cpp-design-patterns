// Pattern: Thread pool / work queue
//
// Backend relevance:
// - A fixed set of worker threads pulls work from a queue.
// - Lets you limit concurrency (protect DB/cache), smooth bursts, and keep latency predictable.
// - Central primitive for request handlers, background jobs, and CPU-bound stages.
//
// This example:
// - Implements a small `ThreadPool` with a blocking work queue.
// - `submit()` returns a `std::future<T>` so callers can await results.
// - Demonstrates fan-out (many tasks) and clean shutdown.

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "../common/log.h"

class ThreadPool {
   public:
    explicit ThreadPool(std::size_t threads) {
        if (threads == 0) {
            throw std::invalid_argument("threads must be > 0");
        }
        workers_.reserve(threads);
        for (std::size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this, i] { worker_loop(i); });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mu_);
            stopping_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) {
            t.join();
        }
    }

    template <class Fn, class... Args>
    auto submit(Fn&& fn, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<Fn>, std::decay_t<Args>...>> {
        // `std::decay_t` normalizes the deduced template types into "value-like" types:
        // - strips references/cv-qualifiers
        // - turns arrays/functions into pointers
        //
        // This makes the computed `Result` match how `fn`/`args` will actually be stored when we
        // capture them by value into the queued lambda below (value captures decay too).
        //
        // Example (why it can matter):
        // - A string literal has type `const char[N]`, so `Args...` can deduce as
        //   `const char (&)[N]`.
        // - But when we store it by value in the lambda capture (`a = ...`), it becomes
        //   `const char*`.
        //
        // If an overload set has different return types, using non-decayed types to compute
        // `Result` can pick the wrong overload:
        //   int lookup(const char (&)[6]);          // (1) returns int
        //   std::string lookup(const char*);        // (2) returns string
        //   submit(lookup, "hello");                // captured arg is `const char*` => calls (2)
        // Without decay, `Result` might be computed from (1) but the queued call returns (2),
        // yielding a mismatched `std::future<...>` / `packaged_task<...>` type.
        using Result = std::invoke_result_t<std::decay_t<Fn>, std::decay_t<Args>...>;

        // `std::packaged_task<Result()>` wraps a callable and owns a shared state (think: value or
        // exception storage + a "ready" flag + synchronization):
        // - When the task runs, it stores either the returned `Result` or a thrown exception
        //   into that shared state.
        // - The `std::future<Result>` returned by `get_future()` lets the caller wait for (and
        //   retrieve) that value/exception.
        auto task = std::make_shared<std::packaged_task<Result()>>(
            [f = std::forward<Fn>(fn), ... a = std::forward<Args>(args)]() mutable -> Result {
                return f(a...);
            });

        // `get_future()` must be called at most once per packaged_task.
        std::future<Result> fut = task->get_future();
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (stopping_) {
                throw std::runtime_error("submit on stopped ThreadPool");
            }
            // The queue stores `Job = std::function<void()>`, which requires the queued callable
            // to be copyable. Capturing a `shared_ptr` to the packaged_task keeps this lambda
            // copyable while still transferring work to the worker threads.
            //
            // When a worker calls `(*task)()`, that will:
            // - run the stored callable, and
            // - fulfill the future (or capture the exception for `future.get()` to rethrow).
            queue_.push([task] { (*task)(); });
        }
        cv_.notify_one();
        return fut;
    }

   private:
    using Job = std::function<void()>;

    void worker_loop(std::size_t worker_id) {
        examples::log_line("[worker-", worker_id, "] start");
        for (;;) {
            Job job;
            {
                std::unique_lock<std::mutex> lock(mu_);
                // Sleep until either:
                // - we're stopping (so we can exit), or
                // - there's work available in the queue.
                //
                // `wait()` atomically releases `mu_` while sleeping and re-acquires it before
                // returning. The predicate form safely handles spurious wakeups.
                cv_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
                if (stopping_ && queue_.empty()) {
                    examples::log_line("[worker-", worker_id, "] stop");
                    return;
                }
                // Move one job out while holding the mutex, then run it outside the lock to
                // minimize contention.
                job = std::move(queue_.front());
                queue_.pop();
            }
            examples::log_line("[worker-", worker_id, "] run job");
            job();
        }
    }

    std::mutex mu_;
    std::condition_variable cv_;
    std::queue<Job> queue_;
    bool stopping_{false};
    std::vector<std::thread> workers_;
};

static int expensive_parse(int request_id) {
    // Simulate CPU work.
    int acc = 0;
    for (int i = 0; i < 100'000; ++i) {
        acc = (acc * 131 + request_id + i) % 1'000'003;
    }
    return acc;
}

int main() {
    examples::log_line("Thread pool example: fan-out parsing stage.");

    ThreadPool pool(4);

    std::vector<std::future<int>> results;
    results.reserve(10);

    for (int req_id = 1; req_id <= 10; ++req_id) {
        results.push_back(pool.submit(expensive_parse, req_id));
    }

    long long total = 0;
    for (auto& f : results) {
        total += f.get();
    }
    examples::log_line("Processed 10 requests; checksum=", total);
}
