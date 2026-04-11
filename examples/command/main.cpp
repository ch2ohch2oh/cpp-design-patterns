// Pattern: Command
//
// Backend relevance:
// - Encapsulates actions as objects for queuing, scheduling, and remote execution.
// - Decouples the invoker from the receiver, supporting background workers and retries.
// - Enables batch processing of operations.
//
// This example:
// - Models PostgreSQL operations (insert, update, delete) as commands.
// - Shows command queuing and batch execution.
// - Demonstrates retry logic with reconnection on transient failures.
// Note: In production, failures come from the PostgreSQL client (network issues, etc.),
//       not from command configuration. The retry mechanism handles any exception thrown during
//       execution.

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../common/log.h"

class PostgresClient {
   public:
    PostgresClient(std::string host, int port, std::string dbname)
        : host_(std::move(host)), port_(port), dbname_(std::move(dbname)), connected_(true) {}

    void insert(const std::string& table, const std::string& data) {
        if (!connected_) {
            throw std::runtime_error("postgres connection lost");
        }
        examples::log_line("[PG] INSERT INTO ", table, " VALUES (", data, ")");
    }

    void update(const std::string& table, const std::string& id, const std::string& data) {
        if (!connected_) {
            throw std::runtime_error("postgres connection lost");
        }
        examples::log_line("[PG] UPDATE ", table, " SET ", data, " WHERE id=", id);
    }

    void delete_(const std::string& table, const std::string& id) {
        if (!connected_) {
            throw std::runtime_error("postgres connection lost");
        }
        examples::log_line("[PG] DELETE FROM ", table, " WHERE id=", id);
    }

    void disconnect() {
        connected_ = false;
        examples::log_line("[PG] disconnected from ", host_, ":", port_, "/", dbname_);
    }

    void reconnect() {
        examples::log_line("[PG] reconnecting to ", host_, ":", port_, "/", dbname_, "...");
        connected_ = true;
        examples::log_line("[PG] reconnected");
    }

   private:
    std::string host_;
    int port_;
    std::string dbname_;
    bool connected_;
};

class Command {
   public:
    virtual ~Command() = default;
    virtual void execute(PostgresClient& pg) = 0;
    virtual const char* description() const = 0;

    void execute_with_retry(PostgresClient& pg, int max_attempts = 3) {
        for (int attempt = 1; attempt <= max_attempts; ++attempt) {
            try {
                execute(pg);
                if (attempt > 1) {
                    examples::log_line("succeeded on attempt ", attempt);
                }
                return;
            } catch (const std::exception& e) {
                examples::log_line("attempt ", attempt, " failed: ", e.what());
                if (attempt == max_attempts) {
                    examples::log_line("max retries exceeded, giving up");
                    throw;
                }
                examples::log_line("attempting to reconnect...");
                pg.reconnect();
                examples::log_line("retrying...");
            }
        }
    }
};

class InsertCommand : public Command {
   public:
    InsertCommand(std::string table, std::string data)
        : table_(std::move(table)), data_(std::move(data)) {}

    void execute(PostgresClient& pg) override { pg.insert(table_, data_); }

    const char* description() const override { return "INSERT"; }

   private:
    std::string table_;
    std::string data_;
};

class UpdateCommand : public Command {
   public:
    UpdateCommand(std::string table, std::string id, std::string data)
        : table_(std::move(table)), id_(std::move(id)), data_(std::move(data)) {}

    void execute(PostgresClient& pg) override { pg.update(table_, id_, data_); }

    const char* description() const override { return "UPDATE"; }

   private:
    std::string table_;
    std::string id_;
    std::string data_;
};

class DeleteCommand : public Command {
   public:
    DeleteCommand(std::string table, std::string id)
        : table_(std::move(table)), id_(std::move(id)) {}

    void execute(PostgresClient& pg) override { pg.delete_(table_, id_); }

    const char* description() const override { return "DELETE"; }

   private:
    std::string table_;
    std::string id_;
};

class CommandQueue {
   public:
    void add_command(std::unique_ptr<Command> cmd) { commands_.push_back(std::move(cmd)); }

    void execute_all(PostgresClient& pg) {
        examples::log_line("executing batch of ", commands_.size(), " commands");
        for (auto& cmd : commands_) {
            cmd->execute(pg);
        }
        commands_.clear();
    }

   private:
    std::vector<std::unique_ptr<Command>> commands_;
};

int main() {
    examples::log_line("Command example: queueable PostgreSQL operations with retry logic.");

    PostgresClient pg("localhost", 5432, "mydb");
    CommandQueue queue;

    // Build a transaction batch
    queue.add_command(std::make_unique<InsertCommand>("users", "(1, 'alice')"));
    queue.add_command(std::make_unique<InsertCommand>("users", "(2, 'bob')"));
    queue.add_command(std::make_unique<UpdateCommand>("users", "1", "name='alice_updated'"));
    queue.add_command(std::make_unique<DeleteCommand>("logs", "42"));

    // Execute the batch
    queue.execute_all(pg);

    // Note: In production, retry logic handles real failures from PostgreSQL client
    // (network issues, connection drops, etc.). The execute_with_retry() method
    // catches any exception and attempts to reconnect before retrying.
}
