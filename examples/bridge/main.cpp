// Pattern: Bridge
//
// Backend relevance:
// - In production, you often need to vary two dimensions independently:
//   - "what you do" (abstraction): formatting, policy, high-level API
//   - "how/where it happens" (implementation): stdout/file, OTLP/console, TCP/UDP, etc.
// - If you combine these dimensions with inheritance alone, you quickly get a class explosion.
// - Bridge avoids that by letting the abstraction hold an "implementor" interface.
// - This specifically addresses the "multiplicity" / N x M problem:
//   e.g. 2 formats x 3 sinks = 6 classes without Bridge, but only (2 + 3) with Bridge.
//
// This example:
// - Models a logger where:
//   - Abstraction hierarchy: `Logger` -> `TextLogger` / `JsonLogger`
//   - Implementor hierarchy: `LogSink` -> `StdoutSink` / `InMemorySink`
// - Demonstrates mixing-and-matching without creating N x M classes.

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../common/log.h"

struct LogField {
    std::string key;
    std::string value;
};

struct LogEvent {
    std::string_view level;
    std::string_view message;
    std::vector<LogField> fields;
};

class LogSink {
   public:
    virtual ~LogSink() = default;
    virtual void write_line(std::string_view line) = 0;
};

class StdoutSink : public LogSink {
   public:
    void write_line(std::string_view line) override { examples::log_line(line); }
};

class InMemorySink : public LogSink {
   public:
    void write_line(std::string_view line) override { lines_.emplace_back(line); }

    const std::vector<std::string>& lines() const { return lines_; }

   private:
    std::vector<std::string> lines_;
};

class Logger {
   public:
    explicit Logger(std::shared_ptr<LogSink> sink) : sink_(std::move(sink)) {}
    virtual ~Logger() = default;

    void info(std::string_view message, std::vector<LogField> fields = {}) {
        emit(LogEvent{.level = "INFO", .message = message, .fields = std::move(fields)});
    }

    void warn(std::string_view message, std::vector<LogField> fields = {}) {
        emit(LogEvent{.level = "WARN", .message = message, .fields = std::move(fields)});
    }

   protected:
    virtual std::string format(const LogEvent& event) const = 0;

   private:
    void emit(const LogEvent& event) { sink_->write_line(format(event)); }

    std::shared_ptr<LogSink> sink_;
};

static std::string escape_json(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (const char ch : s) {
        switch (ch) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            default:
                out.push_back(ch);
                break;
        }
    }
    return out;
}

class TextLogger : public Logger {
   public:
    using Logger::Logger;

   protected:
    std::string format(const LogEvent& event) const override {
        std::string out = std::string(event.level) + " " + std::string(event.message);
        for (const auto& field : event.fields) {
            out += " ";
            out += field.key;
            out += "=";
            out += field.value;
        }
        return out;
    }
};

class JsonLogger : public Logger {
   public:
    using Logger::Logger;

   protected:
    std::string format(const LogEvent& event) const override {
        std::string out;
        out += "{";
        out += "\"level\":\"" + escape_json(event.level) + "\"";
        out += ",\"msg\":\"" + escape_json(event.message) + "\"";
        for (const auto& field : event.fields) {
            out += ",\"";
            out += escape_json(field.key);
            out += "\":\"";
            out += escape_json(field.value);
            out += "\"";
        }
        out += "}";
        return out;
    }
};

static void handle_request(Logger& logger, std::string_view route, std::string_view user_id) {
    const auto started = std::chrono::steady_clock::now();

    logger.info("request.start",
                {{"route", std::string(route)}, {"user_id", std::string(user_id)}});

    // Simulate a handler doing some work (no sleeping to keep this example fast).
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - started)
                                .count();

    logger.info("request.done", {{"route", std::string(route)},
                                 {"status", "200"},
                                 {"latency_ms", std::to_string(elapsed_ms)}});
}

int main() {
    examples::log_line("Bridge example: vary logger format independently from sink destination.");

    auto stdout_sink = std::make_shared<StdoutSink>();
    auto memory_sink = std::make_shared<InMemorySink>();

    JsonLogger json_to_stdout(stdout_sink);
    TextLogger text_to_memory(memory_sink);

    handle_request(json_to_stdout, "/users/42", "alice");
    handle_request(text_to_memory, "/health", "anonymous");

    examples::log_line("---- InMemorySink captured lines ----");
    for (const auto& line : memory_sink->lines()) {
        examples::log_line(line);
    }
}
