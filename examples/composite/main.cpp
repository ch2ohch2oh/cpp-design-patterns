// Pattern: Composite
//
// Backend relevance:
// - Many backend subsystems are naturally tree-shaped: routers, middleware stacks, and filter
// graphs.
// - Composite lets you treat a single "handler" and a group of handlers uniformly.
//
// This example:
// - Builds a small HTTP-like router tree:
//   - Leaf nodes are endpoints that handle exactly one path.
//   - Composite nodes group children under a prefix and dispatch to them.
// - Demonstrates the same interface (`Handler`) for both leaves and composites.

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../common/log.h"

struct Request {
    std::string method;
    std::string path;
};

struct Response {
    int status{200};
    std::string body;
};

class Handler {
   public:
    virtual ~Handler() = default;
    virtual bool try_handle(const Request& request, Response& response) const = 0;
};

class Endpoint final : public Handler {
   public:
    Endpoint(std::string method, std::string path, std::string body)
        : method_(std::move(method)), path_(std::move(path)), body_(std::move(body)) {}

    bool try_handle(const Request& request, Response& response) const override {
        if (request.method != method_ || request.path != path_) {
            return false;
        }
        response.status = 200;
        response.body = body_;
        return true;
    }

   private:
    std::string method_;
    std::string path_;
    std::string body_;
};

static bool starts_with(std::string_view s, std::string_view prefix) {
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

class RouteGroup final : public Handler {
   public:
    explicit RouteGroup(std::string prefix) : prefix_(std::move(prefix)) {}

    void add(std::unique_ptr<Handler> child) { children_.push_back(std::move(child)); }

    bool try_handle(const Request& request, Response& response) const override {
        if (!starts_with(request.path, prefix_)) {
            return false;
        }

        for (const auto& child : children_) {
            if (child->try_handle(request, response)) {
                return true;
            }
        }
        return false;
    }

   private:
    std::string prefix_;
    std::vector<std::unique_ptr<Handler>> children_;
};

static Response dispatch(const Handler& root, const Request& request) {
    Response response;
    if (!root.try_handle(request, response)) {
        response.status = 404;
        response.body = "not found";
    }
    return response;
}

int main() {
    examples::log_line("Composite example: routing tree composed of groups and endpoints.");

    // Build a small routing tree:
    //   /
    //   ├── /health
    //   └── /v1
    //       ├── /v1/users
    //       └── /v1/users/42
    RouteGroup root("/");
    root.add(std::make_unique<Endpoint>("GET", "/health", "ok"));

    auto v1 = std::make_unique<RouteGroup>("/v1");
    v1->add(std::make_unique<Endpoint>("GET", "/v1/users", "list users"));
    v1->add(std::make_unique<Endpoint>("GET", "/v1/users/42", "user 42"));
    root.add(std::move(v1));

    const std::vector<Request> requests{
        {.method = "GET", .path = "/health"},      {.method = "GET", .path = "/v1/users"},
        {.method = "GET", .path = "/v1/users/42"}, {.method = "POST", .path = "/v1/users"},
        {.method = "GET", .path = "/v2/users"},
    };

    for (const auto& req : requests) {
        const Response res = dispatch(root, req);
        examples::log_line(req.method, " ", req.path, " -> ", res.status, " ", res.body);
    }
}
