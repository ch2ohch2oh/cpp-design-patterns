// Pattern: Reactor (event loop)
//
// Backend relevance:
// - A reactor waits for readiness events on many descriptors, then dispatches handlers.
// - This is the core shape behind epoll/kqueue-style network servers.
// - One thread can multiplex many connections without blocking on any single one.
//
// Event loop vs reactor:
// - An event loop is the generic control structure: wait, then dispatch.
// - A reactor is the more specific I/O pattern built on that loop:
//   it waits for descriptor readiness and routes each ready source to its handler.
//
// A realistic reactor loop is still mostly about I/O readiness. The useful work is that one
// thread can manage many connections/timers/wakeup signals at once, then hand off expensive
// application work elsewhere.
//
// Typical shape:
//   wait for readiness on sockets/timers/wakeup fd
//     -> accept new connections
//     -> read ready sockets
//     -> parse frames / update connection state
//     -> hand CPU-heavy work to a thread pool if needed
//     -> flush pending writes on writable sockets
//     -> close errored / timed-out / EOF connections
//
// This example:
// - Creates two pipes that stand in for independent event sources.
// - Uses `poll()` to wait until either source becomes readable.
// - Dispatches the ready descriptor to a small handler map.

#include <poll.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../common/log.h"

// Move-only RAII owner for a Unix file descriptor.
class UniqueFd {
   public:
    explicit UniqueFd(int fd = -1) : fd_(fd) {}

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }

    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        reset();
        fd_ = other.fd_;
        other.fd_ = -1;
        return *this;
    }

    ~UniqueFd() { reset(); }

    int get() const { return fd_; }

    void reset(int fd = -1) {
        if (fd_ != -1) {
            ::close(fd_);
        }
        fd_ = fd;
    }

   private:
    int fd_{-1};
};

struct PipePair {
    UniqueFd read_end;
    UniqueFd write_end;
};

static PipePair make_pipe() {
    int fds[2];
    // POSIX `::pipe(fds)` creates a unidirectional byte stream:
    // - `fds[0]` is the read end
    // - `fds[1]` is the write end
    if (::pipe(fds) != 0) {
        throw std::runtime_error(std::string("pipe failed: ") + std::strerror(errno));
    }
    return PipePair{UniqueFd(fds[0]), UniqueFd(fds[1])};
}

static void write_message(int fd, std::string_view message) {
    const char* data = message.data();
    std::size_t remaining = message.size();
    while (remaining > 0) {
        const ssize_t written = ::write(fd, data, remaining);
        if (written < 0) {
            throw std::runtime_error(std::string("write failed: ") + std::strerror(errno));
        }
        data += written;
        remaining -= static_cast<std::size_t>(written);
    }
}

static std::string describe_revents(short revents) {
    std::string out;
    auto append_flag = [&](std::string_view name) {
        if (!out.empty()) {
            out += "|";
        }
        out += name;
    };

    if ((revents & POLLIN) != 0) {
        append_flag("POLLIN");
    }
    if ((revents & POLLHUP) != 0) {
        append_flag("POLLHUP");
    }
    if ((revents & POLLERR) != 0) {
        append_flag("POLLERR");
    }
    if (out.empty()) {
        out = "0";
    }
    return out;
}

static void simulate_source(UniqueFd fd, std::string source_name,
                            std::vector<std::pair<int, std::string>> events) {
    for (const auto& [delay_ms, payload] : events) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        write_message(fd.get(), payload + "\n");
        examples::log_line("[", source_name, "] emitted ", payload);
    }
    examples::log_line("[", source_name, "] writer done; fd closes via RAII");
}

static bool drain_one_message(int fd, std::string_view source_name) {
    std::string message;
    char ch = '\0';
    while (true) {
        const ssize_t n = ::read(fd, &ch, 1);
        if (n == 0) {
            if (!message.empty()) {
                examples::log_line("[reactor] ", source_name, " -> ", message);
            }
            examples::log_line("[reactor] ", source_name, " -> EOF");
            return false;
        }
        if (n < 0) {
            throw std::runtime_error(std::string("read failed: ") + std::strerror(errno));
        }
        if (ch == '\n') {
            examples::log_line("[reactor] ", source_name, " -> ", message);
            return true;
        }
        message.push_back(ch);
    }
}

int main() {
    examples::log_line("Reactor example: one loop dispatches multiple event sources.");

    PipePair api = make_pipe();
    PipePair metrics = make_pipe();

    std::thread api_writer([fd = std::move(api.write_end)]() mutable {
        simulate_source(std::move(fd), "api-source",
                        {{20, "GET /health"}, {60, "GET /user/42"}, {40, "GET /metrics"}});
    });
    std::thread metrics_writer([fd = std::move(metrics.write_end)]() mutable {
        simulate_source(std::move(fd), "metrics-source",
                        {{35, "flush counters"}, {70, "rotate histogram"}});
    });

    // `pollfd` is the OS watch record for one descriptor:
    // - `fd`     = which descriptor to watch
    // - `events` = what readiness we care about
    // - `revents`= what `poll()` observed on return
    // Here `POLLIN` means "wake me when this descriptor becomes readable".
    std::vector<pollfd> watchers{
        {.fd = api.read_end.get(), .events = POLLIN, .revents = 0},
        {.fd = metrics.read_end.get(), .events = POLLIN, .revents = 0},
    };

    std::unordered_map<int, std::function<bool(int)>> handlers{
        {api.read_end.get(), [&](int fd) { return drain_one_message(fd, "api-handler"); }},
        {metrics.read_end.get(), [&](int fd) { return drain_one_message(fd, "metrics-handler"); }},
    };

    int active_sources = static_cast<int>(watchers.size());
    while (active_sources > 0) {
        // `poll()` blocks until one or more watched descriptors become ready, then fills in
        // each watcher's `revents`.
        // The final argument is a timeout in milliseconds:
        // - 500 => wait up to half a second
        // - 0   => return immediately
        // - -1  => wait forever
        examples::log_line("[reactor] waiting on ", active_sources, " active source(s)");
        const int ready = ::poll(watchers.data(), watchers.size(), 500);
        if (ready < 0) {
            throw std::runtime_error(std::string("poll failed: ") + std::strerror(errno));
        }
        if (ready == 0) {
            // Timeout expired without any fd becoming ready.
            examples::log_line("[reactor] idle tick");
            continue;
        }
        examples::log_line("[reactor] poll returned ", ready, " ready descriptor(s)");

        for (auto& watcher : watchers) {
            // `revents` is a bitmask populated by `poll()` for this descriptor:
            // - `POLLIN`  => readable data is available
            // - `POLLHUP` => the other end closed the pipe/socket
            // - `POLLERR` => the descriptor hit an error condition
            //
            // We skip descriptors that are already deactivated (`fd == -1`) or had no relevant
            // event bits set in this iteration.
            if (watcher.fd == -1 || (watcher.revents & (POLLIN | POLLHUP | POLLERR)) == 0) {
                continue;
            }
            examples::log_line("[reactor] dispatch fd=", watcher.fd,
                               " revents=", describe_revents(watcher.revents));
            const bool keep_open = handlers.at(watcher.fd)(watcher.fd);
            if (!keep_open) {
                // Mark this watch slot inactive so future `poll()` calls ignore it.
                examples::log_line("[reactor] retiring fd=", watcher.fd);
                ::close(watcher.fd);
                watcher.fd = -1;
                --active_sources;
            }
        }
    }

    api_writer.join();
    metrics_writer.join();
}
