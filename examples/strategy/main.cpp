// Pattern: Strategy
//
// Backend relevance:
// - Many backend decisions are policies: retry behavior, routing choice,
//   load balancing, parsing, throttling, etc.
// - Strategy keeps that policy swappable without rewriting the caller.
//
// This example:
// - Models a client that chooses a backend using a pluggable routing strategy.
// - Shows how the same request loop behaves differently with round-robin vs
//   "least loaded" routing.

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "../common/log.h"

struct Backend {
    std::string name;
    int in_flight{0};
};

class RoutingStrategy {
   public:
    virtual ~RoutingStrategy() = default;
    virtual Backend& choose_backend(std::vector<Backend>& backends) = 0;
};

class RoundRobinStrategy : public RoutingStrategy {
   public:
    Backend& choose_backend(std::vector<Backend>& backends) override {
        Backend& chosen = backends[next_ % backends.size()];
        ++next_;
        return chosen;
    }

   private:
    std::size_t next_{0};
};

class LeastLoadedStrategy : public RoutingStrategy {
   public:
    Backend& choose_backend(std::vector<Backend>& backends) override {
        // This strategy ignores order and instead picks the server with the
        // smallest current load.
        return *std::min_element(
            backends.begin(), backends.end(),
            [](const Backend& a, const Backend& b) { return a.in_flight < b.in_flight; });
    }
};

class RequestRouter {
   public:
    explicit RequestRouter(RoutingStrategy& strategy) : strategy_(strategy) {}

    void handle_request(std::vector<Backend>& backends, int request_id, int simulated_work) {
        Backend& backend = strategy_.choose_backend(backends);
        backend.in_flight += simulated_work;

        examples::log_line("request=", request_id, " -> ", backend.name,
                           " new_in_flight=", backend.in_flight);
    }

   private:
    RoutingStrategy& strategy_;
};

static std::vector<Backend> make_backends() {
    return {
        Backend{.name = "api-a", .in_flight = 2},
        Backend{.name = "api-b", .in_flight = 5},
        Backend{.name = "api-c", .in_flight = 1},
    };
}

static void run_demo(const std::string& label, RoutingStrategy& strategy) {
    examples::log_line(label);

    std::vector<Backend> backends = make_backends();
    RequestRouter router(strategy);

    router.handle_request(backends, 101, 2);
    router.handle_request(backends, 102, 2);
    router.handle_request(backends, 103, 2);
}

int main() {
    examples::log_line("Strategy example: swap routing policy without changing caller.");

    RoundRobinStrategy round_robin;
    LeastLoadedStrategy least_loaded;

    run_demo("round robin:", round_robin);
    run_demo("least loaded:", least_loaded);
}
