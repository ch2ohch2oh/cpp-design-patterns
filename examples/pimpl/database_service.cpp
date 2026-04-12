#include "database_service.h"

#include "../common/log.h"

// Implementation - could have heavy dependencies here.
// In a real project, this is where DB drivers, TLS libraries, etc. would be included.
struct DatabaseService::Impl {
    std::string connection_string_;
    std::vector<std::string> results_cache_;

    explicit Impl(std::string connection_string)
        : connection_string_(std::move(connection_string)) {
        examples::log_line("Impl: connecting to database: ", connection_string_);
        // In real code: heavy initialization like SSL setup, connection pools, etc.
    }

    ~Impl() { examples::log_line("Impl: closing database connection"); }

    void execute(const std::string& query) {
        examples::log_line("Impl: executing query: ", query);
        // Simulate query execution.
        results_cache_.push_back("result for: " + query);
    }

    std::vector<std::string> fetch() const { return results_cache_; }
};

// Constructor - create the implementation.
DatabaseService::DatabaseService(std::string connection_string)
    : pimpl_(std::make_unique<Impl>(std::move(connection_string))) {}

// Destructor - must be defined where Impl is complete.
DatabaseService::~DatabaseService() = default;

// Move operations.
DatabaseService::DatabaseService(DatabaseService&&) noexcept = default;
DatabaseService& DatabaseService::operator=(DatabaseService&&) noexcept = default;

// Public methods forward to implementation.
void DatabaseService::execute_query(const std::string& query) { pimpl_->execute(query); }

std::vector<std::string> DatabaseService::fetch_results() { return pimpl_->fetch(); }
