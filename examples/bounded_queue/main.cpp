// Pattern: Producer–consumer (bounded queue)
//
// Backend relevance:
// - A bounded queue provides backpressure: producers block when the system is saturated.
// - Prevents unbounded memory growth during traffic spikes.
// - Common between acceptor->worker, parser->handler, and handler->writer stages.
//
// This example:
// - Implements a small blocking `BoundedQueue<T>`.
// - Runs producers that generate "requests" and consumers that process them.
// - Shows clean shutdown by closing the queue.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#if __has_include(<syncstream>)
#include <syncstream>
#endif
#include <thread>
#include <utility>
#include <vector>

namespace {
std::mutex& log_mutex() {
    static std::mutex mu;
    return mu;
}

template <class... Ts>
void log_line(Ts&&... parts) {
#if defined(__cpp_lib_syncbuf) && (__cpp_lib_syncbuf >= 201803L)
    std::osyncstream out(std::cout);
    ((out << std::forward<Ts>(parts)), ...) << "\n";
#else
    // Note: the mutex must be shared across all template instantiations to prevent interleaving.
    std::lock_guard<std::mutex> lock(log_mutex());
    ((std::cout << std::forward<Ts>(parts)), ...) << "\n";
#endif
}
}  // namespace

template <class T>
class BoundedQueue {
   public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument("capacity must be > 0");
        }
    }

    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;

    void close() {
        {
            std::lock_guard<std::mutex> lock(mu_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    // Blocks until enqueued or closed. Returns false if closed.
    bool push(T value) {
        std::unique_lock<std::mutex> lock(mu_);
        not_full_.wait(lock, [this] { return closed_ || q_.size() < capacity_; });
        if (closed_) {
            return false;
        }
        q_.push(std::move(value));
        not_empty_.notify_one();
        return true;
    }

    // Blocks until an item is available or closed+empty. Returns nullopt when drained.
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mu_);
        not_empty_.wait(lock, [this] { return closed_ || !q_.empty(); });
        if (q_.empty()) {
            return std::nullopt;
        }
        T out = std::move(q_.front());
        q_.pop();
        not_full_.notify_one();
        return out;
    }

   private:
    std::mutex mu_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::queue<T> q_;
    std::size_t capacity_{0};
    bool closed_{false};
};

struct WorkItem {
    int request_id{0};
};

static void producer(BoundedQueue<WorkItem>& q, int producer_id, int items) {
    log_line("producer-", producer_id, " start");
    for (int i = 0; i < items; ++i) {
        int req_id = producer_id * 1000 + i;
        if (!q.push(WorkItem{.request_id = req_id})) {
            log_line("producer-", producer_id, " stopped (queue closed)");
            return;
        }
        log_line("producer-", producer_id, " enqueued req=", req_id);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    log_line("producer-", producer_id, " done");
}

static void consumer(BoundedQueue<WorkItem>& q, int consumer_id, std::atomic<int>& processed) {
    for (;;) {
        auto item = q.pop();
        if (!item) {
            return;
        }
        ++processed;
        log_line("worker-", consumer_id, " handled req=", item->request_id);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}

int main() {
    log_line("Bounded queue example: backpressure under load.");

    BoundedQueue<WorkItem> q(4);  // small capacity to make backpressure observable
    std::atomic<int> processed{0};

    std::thread c1([&] { consumer(q, 1, processed); });
    std::thread c2([&] { consumer(q, 2, processed); });

    std::thread p1([&] { producer(q, 1, 6); });
    std::thread p2([&] { producer(q, 2, 6); });

    p1.join();
    p2.join();
    q.close();

    c1.join();
    c2.join();

    log_line("Processed ", processed.load(), " items.");
}
