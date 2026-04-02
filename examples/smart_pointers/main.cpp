// Pattern: Ownership with smart pointers (`std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr`)
//
// Backend relevance:
// - Request handling often involves async work and callbacks; ownership must be
//   explicit to avoid leaks and use-after-free.
// - `unique_ptr` models single ownership (e.g., a response produced by a handler).
// - `shared_ptr` models shared lifetime (e.g., a DB pool shared by many requests).
//
// This example simulates request routing:
// - A `DbPool` is shared across requests (`shared_ptr`).
// - Each handler returns a `unique_ptr<Response>` to make ownership transfer explicit.

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
    RequestContext(Request request, std::shared_ptr<const DbPool> db)
        : request_(std::move(request)), db_(std::move(db)) {}

    const Request& request() const { return request_; }
    const DbPool& db() const { return *db_; }

   private:
    Request request_;
    // Shared because the pool is long-lived and referenced by many concurrent requests.
    std::shared_ptr<const DbPool> db_;
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

    Request req1{.method = "GET", .path = "/health"};
    Request req2{.method = "GET", .path = "/user/42"};

    for (const auto& req : {req1, req2}) {
        RequestContext ctx(req, db_pool);
        std::unique_ptr<Response> res = dispatch(ctx);
        std::cout << req.method << " " << req.path << " -> " << res->status << " " << res->body
                  << "\n";
    }
}
