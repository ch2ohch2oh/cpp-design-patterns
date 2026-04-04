// Pattern: State
//
// Backend relevance:
// - Protocols, sessions, and connections naturally move through states.
// - State replaces a growing `switch` over mode flags with explicit state objects.
// - Each state owns the behavior that is valid for that phase.
//
// This example:
// - Models a tiny connection lifecycle with `Disconnected`, `Authenticating`,
//   and `Ready` states.
// - Routes the same operations through different behavior depending on state.
// - Shows invalid actions being handled by the current state instead of scattered
//   conditionals.

#include <memory>
#include <string>
#include <utility>

#include "../common/log.h"

class Connection;

class ConnectionState {
   public:
    virtual ~ConnectionState() = default;

    virtual const char* name() const = 0;
    virtual void connect(Connection& connection) = 0;
    virtual void authenticate(Connection& connection, std::string token) = 0;
    virtual void send_query(Connection& connection, std::string sql) = 0;
};

class DisconnectedState;
class AuthenticatingState;
class ReadyState;

class Connection {
   public:
    Connection();

    void connect() { state_->connect(*this); }

    void authenticate(std::string token) { state_->authenticate(*this, std::move(token)); }

    void send_query(std::string sql) { state_->send_query(*this, std::move(sql)); }

    void transition_to(std::unique_ptr<ConnectionState> next_state) {
        examples::log_line("state transition: ", state_->name(), " -> ", next_state->name());
        state_ = std::move(next_state);
    }

   private:
    std::unique_ptr<ConnectionState> state_;
};

class DisconnectedState : public ConnectionState {
   public:
    const char* name() const override { return "Disconnected"; }

    void connect(Connection& connection) override;
    void authenticate(Connection&, std::string) override {
        examples::log_line("cannot authenticate before connecting");
    }
    void send_query(Connection&, std::string sql) override {
        examples::log_line("dropping query while disconnected: ", sql);
    }
};

class AuthenticatingState : public ConnectionState {
   public:
    const char* name() const override { return "Authenticating"; }

    void connect(Connection&) override {
        examples::log_line("connection already established; waiting for auth");
    }
    void authenticate(Connection& connection, std::string token) override;
    void send_query(Connection&, std::string sql) override {
        examples::log_line("cannot run query before auth completes: ", sql);
    }
};

class ReadyState : public ConnectionState {
   public:
    const char* name() const override { return "Ready"; }

    void connect(Connection&) override { examples::log_line("connection already ready"); }
    void authenticate(Connection&, std::string) override {
        examples::log_line("already authenticated");
    }
    void send_query(Connection&, std::string sql) override {
        examples::log_line("executing query: ", sql);
    }
};

Connection::Connection() : state_(std::make_unique<DisconnectedState>()) {}

void DisconnectedState::connect(Connection& connection) {
    examples::log_line("opening socket and starting handshake");
    connection.transition_to(std::make_unique<AuthenticatingState>());
}

void AuthenticatingState::authenticate(Connection& connection, std::string token) {
    examples::log_line("validating token=", token);
    connection.transition_to(std::make_unique<ReadyState>());
}

int main() {
    examples::log_line("State example: connection behavior depends on current lifecycle state.");

    Connection connection;

    connection.send_query("SELECT 1");
    connection.connect();
    connection.send_query("SELECT * FROM users");
    connection.authenticate("token-abc");
    connection.send_query("SELECT * FROM users");
}
