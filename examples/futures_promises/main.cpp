// Pattern: Futures/promises (task-based async)
//
// Backend relevance:
// - Lets you express "start work now, consume result later" without blocking a request thread.
// - Useful for parallel IO (cache + DB), fan-out, timeouts, and background jobs.
// - A `std::promise<T>` is a write-end for a result; `std::future<T>` is the read-end.
//
// This example:
// - Runs "cache" and "db" lookups concurrently.
// - Uses a promise to return the first successful result (or the last error if both fail).
// - Demonstrates `wait_for()` as a building block for timeouts.

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include "../common/log.h"

static std::string cache_lookup(int user_id) {
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    if (user_id == 42) {
        return "Ada (cache)";
    }
    throw std::runtime_error("cache miss");
}

static std::string db_lookup(int user_id) {
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    return user_id == 42 ? "Ada (db)" : "unknown (db)";
}

struct FirstSuccess {
    std::promise<std::string> promise;
    std::atomic<bool> done{false};
    std::atomic<int> remaining{0};
    std::mutex mu;
    std::exception_ptr last_error;

    explicit FirstSuccess(int n) : remaining(n) {}

    void try_set_value(std::string v) {
        if (!done.exchange(true)) {
            promise.set_value(std::move(v));
        }
    }

    void report_failure(std::exception_ptr e) {
        // Record this failure. We keep only the "last" error for simplicity (any error is fine
        // to return once we've established that all lookups failed).
        {
            std::lock_guard<std::mutex> lock(mu);
            last_error = std::move(e);
        }

        // `remaining` tracks how many outstanding operations might still be able to succeed.
        // Only failures decrement it.
        //
        // If this was the *last failure* (i.e. `remaining` transitions 1 -> 0), then all lookups
        // have failed, so we should complete the promise with an error.
        //
        // Important: we still must check `done` because a successful lookup might have already
        // fulfilled the promise. In that case, the last failing thread must NOT try to
        // `set_exception()` (promise already satisfied).
        if (remaining.fetch_sub(1) == 1) {
            if (!done.exchange(true)) {
                std::exception_ptr err;
                {
                    std::lock_guard<std::mutex> lock(mu);
                    err = last_error
                              ? last_error
                              : std::make_exception_ptr(std::runtime_error("all lookups failed"));
                }
                promise.set_exception(std::move(err));
            }
        }
    }
};

template <class Fn>
static std::thread run_lookup(std::shared_ptr<FirstSuccess> state, Fn fn) {
    return std::thread([state = std::move(state), fn = std::move(fn)]() mutable {
        try {
            state->try_set_value(fn());
        } catch (...) {
            state->report_failure(std::current_exception());
        }
    });
}

static void run_lookup_demo(int user_id) {
    auto state = std::make_shared<FirstSuccess>(2);
    std::future<std::string> result = state->promise.get_future();

    std::thread t1 = run_lookup(state, [=] { return cache_lookup(user_id); });
    std::thread t2 = run_lookup(state, [=] { return db_lookup(user_id); });

    // Timeout building block: don't block forever.
    if (result.wait_for(std::chrono::milliseconds(200)) != std::future_status::ready) {
        examples::log_line("Timed out waiting for user lookup (user=", user_id, ").");
    } else {
        try {
            examples::log_line("User ", user_id, " name: ", result.get());
        } catch (const std::exception& e) {
            examples::log_line("User ", user_id, " lookup failed: ", e.what());
        }
    }

    t1.join();
    t2.join();
}

int main() {
    examples::log_line("Futures/promises example: race cache vs db.");

    // Demonstrate both paths:
    // - user=42: cache hit wins quickly
    // - user=7: cache misses; db succeeds later, so "first success" is the db answer
    run_lookup_demo(42);
    run_lookup_demo(7);
}
