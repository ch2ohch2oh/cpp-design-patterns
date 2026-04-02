// Pattern: Ownership with smart pointers (`std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr`)
//
// Backend relevance:
// - Request handling often involves async work and callbacks; ownership must be
//   explicit to avoid leaks and use-after-free.
// - `unique_ptr` models single ownership (e.g., a response produced by a handler).
// - `shared_ptr` models shared lifetime (e.g., a DB pool shared by many requests).
// - `weak_ptr` models a non-owning reference (e.g., optional observers like tracing/metrics)
//   without artificially extending lifetimes or creating reference cycles.
//
// This example simulates request routing:
// - A `DbPool` is shared across requests (`shared_ptr`).
// - A `Tracer` is referenced non-owningly (`weak_ptr`) to demonstrate `lock()` + expiration.
// - Each handler returns a `unique_ptr<Response>` to make ownership transfer explicit.
//
// How to create a `weak_ptr`:
// - You create it from a `shared_ptr` (or another `weak_ptr`), not from a `unique_ptr`.
//   Example:
//     auto sp = std::make_shared<Foo>();
//     std::weak_ptr<Foo> wp = sp;

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

struct Request {
    std::string method;
    std::string path;
};

struct Response {
    int status{200};
    std::string body;
};

class Tracer {
   public:
    void record(std::string_view event) const { std::cout << "[trace] " << event << "\n"; }
};

class DbPool {
   public:
    explicit DbPool(std::string dsn) : dsn_(std::move(dsn)) {}
    std::string_view dsn() const { return dsn_; }

    std::string query_user_name(int user_id) const { return user_id == 42 ? "Ada" : "unknown"; }

   private:
    std::string dsn_;
};

class RequestContext {
   public:
    RequestContext(Request request, std::shared_ptr<const DbPool> db,
                   std::weak_ptr<const Tracer> tracer)
        : request_(std::move(request)), db_(std::move(db)), tracer_(std::move(tracer)) {}

    const Request& request() const { return request_; }
    const DbPool& db() const { return *db_; }
    // `weak_ptr::lock()` attempts to "promote" a non-owning reference to a temporary `shared_ptr`.
    // - If the object is still alive, you get a non-null `shared_ptr` that keeps it alive while
    // used.
    // - If it already expired, you get an empty `shared_ptr` (check it before dereferencing).
    std::shared_ptr<const Tracer> tracer() const { return tracer_.lock(); }

   private:
    Request request_;
    // Shared because the pool is long-lived and referenced by many concurrent requests.
    std::shared_ptr<const DbPool> db_;
    // Non-owning: requests can emit traces if the tracer is still alive, but they don't keep it
    // alive.
    std::weak_ptr<const Tracer> tracer_;
};

// A minimal "handler" signature:
// - Takes context by reference (no ownership).
// - Returns a `unique_ptr<Response>` (caller becomes the owner).
using Handler = std::unique_ptr<Response> (*)(const RequestContext&);

static std::unique_ptr<Response> handle_health(const RequestContext&) {
    auto response = std::make_unique<Response>();
    response->status = 200;
    response->body = "ok";
    return response;
}

static std::unique_ptr<Response> handle_user(const RequestContext& ctx) {
    if (auto tracer = ctx.tracer()) {
        tracer->record("handle_user");
    }
    auto response = std::make_unique<Response>();
    response->status = 200;
    response->body = "user=42 name=" + ctx.db().query_user_name(42);
    return response;
}

static std::unique_ptr<Response> handle_not_found(const RequestContext&) {
    auto response = std::make_unique<Response>();
    response->status = 404;
    response->body = "not found";
    return response;
}

static std::unique_ptr<Response> dispatch(const RequestContext& ctx) {
    static const std::unordered_map<std::string, Handler> routes{
        {"/health", &handle_health},
        {"/user/42", &handle_user},
    };

    auto it = routes.find(ctx.request().path);
    if (it == routes.end()) return handle_not_found(ctx);
    // Returning a `unique_ptr` makes it impossible to accidentally "share" a response
    // across requests without explicitly opting in.
    return it->second(ctx);
}

int main() {
    auto db_pool = std::make_shared<DbPool>("postgres://example");
    auto tracer = std::make_shared<Tracer>();

    Request req1{.method = "GET", .path = "/health"};
    Request req2{.method = "GET", .path = "/user/42"};

    for (const auto& req : {req1, req2}) {
        RequestContext ctx(req, db_pool, tracer);
        std::unique_ptr<Response> res = dispatch(ctx);
        std::cout << req.method << " " << req.path << " -> " << res->status << " " << res->body
                  << "\n";
    }

    // Demonstrate expiration: requests can hold a weak reference without keeping the tracer alive.
    std::weak_ptr<const Tracer> tracer_weak = tracer;
    tracer.reset();
    if (!tracer_weak.lock()) {
        std::cout << "[trace] tracer expired (weak_ptr did not extend lifetime)\n";
    }
}
