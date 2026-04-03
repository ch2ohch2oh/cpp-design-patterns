// Pattern: Decorator
//
// Backend relevance:
// - Decorator layers behavior around an object while keeping the same interface.
// - Useful for logging, metrics, authorization, tracing, retries, and timing.
//
// This example:
// - Starts with a simple request handler.
// - Wraps it first with logging, then with timing.
// - Each decorator adds behavior before/after delegating to the wrapped handler.
//
// Intuition:
// - Yes, a decorator is a wrapper.
// - But not every wrapper is a decorator.
// - A decorator keeps the same interface as the wrapped object, forwards calls to it,
//   and adds behavior around those calls.

#include <chrono>
#include <memory>
#include <string>

#include "../common/log.h"

class RequestHandler {
   public:
    virtual ~RequestHandler() = default;
    virtual std::string handle(std::string request) = 0;
};

class EchoHandler : public RequestHandler {
   public:
    std::string handle(std::string request) override { return "200 OK: " + request; }
};

class HandlerDecorator : public RequestHandler {
   public:
    explicit HandlerDecorator(std::unique_ptr<RequestHandler> inner) : inner_(std::move(inner)) {}

   protected:
    // This is what makes the wrapper a decorator rather than just a helper object:
    // it stores another `RequestHandler`, and it is itself also a `RequestHandler`.
    RequestHandler& inner() { return *inner_; }

   private:
    std::unique_ptr<RequestHandler> inner_;
};

class LoggingDecorator : public HandlerDecorator {
   public:
    using HandlerDecorator::HandlerDecorator;

    std::string handle(std::string request) override {
        examples::log_line("logging: request=", request);
        std::string response = inner().handle(request);
        examples::log_line("logging: response=", response);
        return response;
    }
};

class TimingDecorator : public HandlerDecorator {
   public:
    using HandlerDecorator::HandlerDecorator;

    std::string handle(std::string request) override {
        auto start = std::chrono::steady_clock::now();
        std::string response = inner().handle(request);
        auto end = std::chrono::steady_clock::now();

        auto elapsed_us =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        examples::log_line("timing: took ", elapsed_us, "us");
        return response;
    }
};

int main() {
    examples::log_line("Decorator example: stack logging and timing around a handler.");

    std::unique_ptr<RequestHandler> handler = std::make_unique<TimingDecorator>(
        std::make_unique<LoggingDecorator>(std::make_unique<EchoHandler>()));

    std::string response = handler->handle("GET /users/42");
    examples::log_line("final response=", response);
}
