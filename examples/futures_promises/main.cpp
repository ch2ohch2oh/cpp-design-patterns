// Pattern: Futures/promises (task-based async)
//
// Backend relevance:
// - Lets you express "start work now, consume result later" without blocking a request thread.
// - Useful for parallel IO (cache + DB), fan-out, timeouts, and background jobs.
// - A `std::promise<T>` is a write-end for a result; `std::future<T>` is the read-end.
//
// This example:
// - Runs "cache" and "db" lookups concurrently.
// - Uses a promise to return the first successful result (or propagate an error).
// - Demonstrates `wait_for()` as a building block for timeouts.

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

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

template <class Fn>
static std::thread run_and_set_first(std::shared_ptr<std::atomic<bool>> done,
                                     std::shared_ptr<std::promise<std::string>> out, Fn fn) {
    return std::thread(
        [done = std::move(done), out = std::move(out), fn = std::move(fn)]() mutable {
            try {
                std::string v = fn();
                if (!done->exchange(true)) {
                    out->set_value(std::move(v));
                }
            } catch (...) {
                if (!done->exchange(true)) {
                    out->set_exception(std::current_exception());
                }
            }
        });
}

int main() {
    std::cout << "Futures/promises example: race cache vs db.\n";

    auto done = std::make_shared<std::atomic<bool>>(false);
    auto p = std::make_shared<std::promise<std::string>>();
    std::future<std::string> result = p->get_future();

    int user_id = 42;
    std::thread t1 = run_and_set_first(done, p, [=] { return cache_lookup(user_id); });
    std::thread t2 = run_and_set_first(done, p, [=] { return db_lookup(user_id); });

    // Timeout building block: don't block forever.
    if (result.wait_for(std::chrono::milliseconds(200)) != std::future_status::ready) {
        std::cout << "Timed out waiting for user lookup.\n";
    } else {
        try {
            std::cout << "User name: " << result.get() << "\n";
        } catch (const std::exception& e) {
            std::cout << "Lookup failed: " << e.what() << "\n";
        }
    }

    t1.join();
    t2.join();
}
