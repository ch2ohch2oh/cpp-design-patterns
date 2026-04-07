// Pattern: State
//
// Backend relevance:
// - Protocols, sessions, and connections naturally move through states.
// - State replaces a growing `switch` over mode flags with explicit state objects.
// - Each state owns the behavior that is valid for that phase.
//
// This example:
// - Models a tiny connection lifecycle with `Disconnected`, `Connecting`,
//   `Authenticating`, and `Ready` states.
// - Routes both API calls and IO events through different behavior depending on state.
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
    virtual void on_socket_connected(Connection& connection) = 0;
    virtual void on_bytes_received(Connection& connection, std::string bytes) = 0;
    virtual void on_socket_closed(Connection& connection, std::string reason) = 0;
};

class DisconnectedState;
class ConnectingState;
class AuthenticatingState;
class ReadyState;

class Connection {
   public:
    Connection();

    void connect() { state_->connect(*this); }

    void authenticate(std::string token) { state_->authenticate(*this, std::move(token)); }

    void send_query(std::string sql) { state_->send_query(*this, std::move(sql)); }

    void on_socket_connected() { state_->on_socket_connected(*this); }

    void on_bytes_received(std::string bytes) {
        state_->on_bytes_received(*this, std::move(bytes));
    }

    void on_socket_closed(std::string reason) {
        state_->on_socket_closed(*this, std::move(reason));
    }

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
    void on_socket_connected(Connection&) override {
        examples::log_line("unexpected socket_connected while disconnected");
    }
    void on_bytes_received(Connection&, std::string bytes) override {
        examples::log_line("unexpected bytes while disconnected: ", bytes);
    }
    void on_socket_closed(Connection&, std::string reason) override {
        examples::log_line("socket already closed: ", reason);
    }
};

class ConnectingState : public ConnectionState {
   public:
    const char* name() const override { return "Connecting"; }

    void connect(Connection&) override { examples::log_line("already connecting"); }
    void authenticate(Connection&, std::string) override {
        examples::log_line("cannot authenticate until socket connects");
    }
    void send_query(Connection&, std::string sql) override {
        examples::log_line("cannot send query until connected: ", sql);
    }
    void on_socket_connected(Connection& connection) override;
    void on_bytes_received(Connection&, std::string bytes) override {
        examples::log_line("dropping bytes while connecting: ", bytes);
    }
    void on_socket_closed(Connection& connection, std::string reason) override;
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
    void on_socket_connected(Connection&) override {
        examples::log_line("socket already connected; waiting for auth");
    }
    void on_bytes_received(Connection& connection, std::string bytes) override;
    void on_socket_closed(Connection& connection, std::string reason) override;
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
    void on_socket_connected(Connection&) override {
        examples::log_line("socket already connected");
    }
    void on_bytes_received(Connection&, std::string bytes) override {
        examples::log_line("received server bytes: ", bytes);
    }
    void on_socket_closed(Connection& connection, std::string reason) override;
};

Connection::Connection() : state_(std::make_unique<DisconnectedState>()) {}

void DisconnectedState::connect(Connection& connection) {
    examples::log_line("starting async connect");
    connection.transition_to(std::make_unique<ConnectingState>());
}

void AuthenticatingState::authenticate(Connection& connection, std::string token) {
    (void)connection;
    examples::log_line("sending auth token=", token, " and waiting for server response");
}

void ConnectingState::on_socket_connected(Connection& connection) {
    examples::log_line("tcp connected; beginning authentication");
    connection.transition_to(std::make_unique<AuthenticatingState>());
}

void ConnectingState::on_socket_closed(Connection& connection, std::string reason) {
    examples::log_line("connect failed: ", reason);
    connection.transition_to(std::make_unique<DisconnectedState>());
}

void AuthenticatingState::on_bytes_received(Connection& connection, std::string bytes) {
    if (bytes == "AUTH_OK") {
        examples::log_line("auth succeeded");
        connection.transition_to(std::make_unique<ReadyState>());
        return;
    }

    if (bytes == "AUTH_FAIL") {
        examples::log_line("auth failed");
        connection.transition_to(std::make_unique<DisconnectedState>());
        return;
    }

    examples::log_line("ignoring unexpected auth response: ", bytes);
}

void AuthenticatingState::on_socket_closed(Connection& connection, std::string reason) {
    examples::log_line("socket closed during auth: ", reason);
    connection.transition_to(std::make_unique<DisconnectedState>());
}

void ReadyState::on_socket_closed(Connection& connection, std::string reason) {
    examples::log_line("socket closed: ", reason);
    connection.transition_to(std::make_unique<DisconnectedState>());
}

int main() {
    examples::log_line("State example: connection behavior depends on current lifecycle state.");

    Connection connection;

    connection.send_query("SELECT 1");
    connection.connect();
    connection.send_query("SELECT * FROM users");
    connection.on_socket_connected();
    connection.authenticate("token-abc");
    connection.on_bytes_received("AUTH_OK");
    connection.send_query("SELECT * FROM users");
    connection.on_socket_closed("server hung up");
}
