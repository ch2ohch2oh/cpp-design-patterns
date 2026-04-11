// Pattern: Visitor (via std::variant + std::visit)
//
// Backend relevance:
// - Protocols and message formats often have many variants (requests, responses,
//   errors, heartbeats, etc.).
// - Visitor + std::variant lets you handle each variant cleanly without
//   type-unsafe casts or large switch statements.
//
// This example:
// - Defines a set of protocol messages using std::variant.
// - Implements visitors that process each message type (serialization, logging,
//   dispatch).
// - Shows how std::visit applies the visitor to the active variant.

#include <iostream>
#include <string>
#include <variant>
#include <vector>

#include "../common/log.h"

// Protocol message types
struct Connect {
    std::string client_id;
    int version;
};

struct Publish {
    std::string topic;
    std::string payload;
    int qos;
};

struct Subscribe {
    std::string topic_filter;
};

struct Disconnect {
    std::string reason;
};

// The message variant: can hold any of the above types
using Message = std::variant<Connect, Publish, Subscribe, Disconnect>;

// Visitor 1: Serialize message to string
struct Serializer {
    std::string operator()(const Connect& msg) const {
        return "CONNECT(client=" + msg.client_id + ", v=" + std::to_string(msg.version) + ")";
    }

    std::string operator()(const Publish& msg) const {
        return "PUBLISH(topic=" + msg.topic + ", qos=" + std::to_string(msg.qos) + ")";
    }

    std::string operator()(const Subscribe& msg) const {
        return "SUBSCRIBE(filter=" + msg.topic_filter + ")";
    }

    std::string operator()(const Disconnect& msg) const {
        return "DISCONNECT(reason=" + msg.reason + ")";
    }
};

// Visitor 2: Log message with metadata
struct MessageLogger {
    void operator()(const Connect& msg) const {
        examples::log_line("[CONNECT] Client ", msg.client_id, " version ", msg.version);
    }

    void operator()(const Publish& msg) const {
        examples::log_line("[PUBLISH] Topic ", msg.topic, " size ", msg.payload.size(), " qos ",
                           msg.qos);
    }

    void operator()(const Subscribe& msg) const {
        examples::log_line("[SUBSCRIBE] Filter ", msg.topic_filter);
    }

    void operator()(const Disconnect& msg) const {
        examples::log_line("[DISCONNECT] Reason: ", msg.reason);
    }
};

// Visitor 3: Dispatch to appropriate handler (returns response string)
struct MessageDispatcher {
    std::string operator()(const Connect& msg) const { return "CONNACK: accepted"; }

    std::string operator()(const Publish& msg) const { return "PUBACK: msg_id=123"; }

    std::string operator()(const Subscribe& msg) const { return "SUBACK: granted_qos=1"; }

    std::string operator()(const Disconnect& msg) const { return "Closing connection"; }
};

int main() {
    examples::log_line(
        "Visitor example: handle protocol message variants with std::variant + std::visit.");

    // Create a sequence of messages
    std::vector<Message> messages{
        Connect{.client_id = "client-001", .version = 2},
        Publish{.topic = "sensors/temp", .payload = "22.5", .qos = 1},
        Subscribe{.topic_filter = "sensors/#"},
        Publish{.topic = "sensors/humidity", .payload = "45.2", .qos = 0},
        Disconnect{.reason = "client_shutdown"},
    };

    // Apply visitors to each message
    examples::log_line("\n--- Serialization ---");
    Serializer serializer;
    for (const auto& msg : messages) {
        examples::log_line(std::visit(serializer, msg));
    }

    examples::log_line("\n--- Logging ---");
    MessageLogger logger;
    for (const auto& msg : messages) {
        std::visit(logger, msg);
    }

    examples::log_line("\n--- Dispatch ---");
    MessageDispatcher dispatcher;
    for (const auto& msg : messages) {
        std::string serialized = std::visit(serializer, msg);
        std::string response = std::visit(dispatcher, msg);
        examples::log_line(serialized, " => ", response);
    }

    // === NON-VISITOR EQUIVALENTS ===
    //
    // Approach 1: Manual type checking (verbose, type-unsafe, error-prone)
    //
    // std::string serialize_manual(const Message& msg) {
    //     if (std::holds_alternative<Connect>(msg)) {
    //         const auto& m = std::get<Connect>(msg);
    //         return "CONNECT(client=" + m.client_id + ", v=" + std::to_string(m.version) + ")";
    //     } else if (std::holds_alternative<Publish>(msg)) {
    //         const auto& m = std::get<Publish>(msg);
    //         return "PUBLISH(topic=" + m.topic + ", qos=" + std::to_string(m.qos) + ")";
    //     } else if (std::holds_alternative<Subscribe>(msg)) {
    //         const auto& m = std::get<Subscribe>(msg);
    //         return "SUBSCRIBE(filter=" + m.topic_filter + ")";
    //     } else if (std::holds_alternative<Disconnect>(msg)) {
    //         const auto& m = std::get<Disconnect>(msg);
    //         return "DISCONNECT(reason=" + m.reason + ")";
    //     }
    //     return "UNKNOWN";
    // }
    //
    // Problems:
    // - Repetitive if-else chains
    // - Easy to forget a variant (no compiler help)
    // - Type errors at runtime if you get the wrong type
    // - Adding a new message type requires updating every function
    //
    // Approach 2: Traditional OOP with virtual functions
    //
    // struct Message {
    //     virtual ~Message() = default;
    //     virtual std::string serialize() const = 0;
    //     virtual void log() const = 0;
    //     virtual std::string dispatch() const = 0;
    // };
    //
    // struct Connect : Message {
    //     std::string client_id;
    //     int version;
    //     std::string serialize() const override { /* ... */ }
    //     void log() const override { /* ... */ }
    //     std::string dispatch() const override { /* ... */ }
    // };
    //
    // Problems:
    // - Each message type must implement ALL operations (violates SRP)
    // - Adding a new operation requires modifying every message class
    // - Cannot add operations without touching existing classes
    // - Data and behavior are tightly coupled
    //
    // Visitor advantages:
    // - Separates data structures (message types) from algorithms (visitors)
    // - Adding new operations = new visitor (no changes to message types)
    // - Adding new message types = compiler forces updating all visitors
    // - Type-safe: std::visit guarantees exhaustive handling
}
