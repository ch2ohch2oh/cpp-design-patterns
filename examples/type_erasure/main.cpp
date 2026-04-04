// Pattern: Type erasure
//
// Backend relevance:
// - Backend systems often need a uniform container of handlers, callbacks, or jobs
//   even when each item has a different concrete type.
// - Type erasure hides those concrete types behind one small runtime interface,
//   which keeps APIs simple and avoids pushing templates everywhere.
//
// This example:
// - Builds a small middleware pipeline for a request.
// - Stores three different callable types in one vector:
//   a lambda, a function object, and a class with `operator()`.
// - Runs them uniformly as request mutators.

#include <functional>
#include <string>
#include <vector>

#include "../common/log.h"

struct RequestContext {
    std::string route;
    std::string user;
    bool authenticated{false};
    int trace_id{0};
};

using Middleware = std::function<void(RequestContext&)>;

class RequireAuthentication {
   public:
    void operator()(RequestContext& ctx) const {
        ctx.authenticated = true;
        examples::log_line("auth middleware: user=", ctx.user, " authenticated");
    }
};

struct RouteMetricsTagger {
    void operator()(RequestContext& ctx) const {
        ctx.route += " [metrics-tagged]";
        examples::log_line("metrics middleware: route=", ctx.route);
    }
};

class MiddlewarePipeline {
   public:
    void add(Middleware middleware) { chain_.push_back(std::move(middleware)); }

    void run(RequestContext& ctx) const {
        for (const auto& middleware : chain_) {
            middleware(ctx);
        }
    }

   private:
    std::vector<Middleware> chain_;
};

int main() {
    examples::log_line("Type erasure example: store heterogeneous middleware in one pipeline.");

    RequestContext ctx{
        .route = "/users/42",
        .user = "alice",
        .authenticated = false,
        .trace_id = 7,
    };

    MiddlewarePipeline pipeline;

    pipeline.add([](RequestContext& request) {
        examples::log_line("trace middleware: trace_id=", request.trace_id);
    });
    pipeline.add(RequireAuthentication{});
    pipeline.add(RouteMetricsTagger{});

    pipeline.run(ctx);

    examples::log_line("final context: route=", ctx.route, " user=", ctx.user,
                       " authenticated=", ctx.authenticated);
}
