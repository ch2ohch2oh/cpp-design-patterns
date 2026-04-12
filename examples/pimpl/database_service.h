#pragma once

#include <memory>
#include <string>
#include <vector>

// Public API - only what users need to see.
class DatabaseService {
   public:
    // Forward declaration - users of DatabaseService don't need to know Impl's details.
    struct Impl;

   public:
    // Constructor: creates the implementation.
    explicit DatabaseService(std::string connection_string);

    // Destructor: defined in .cpp to ensure Impl is complete.
    ~DatabaseService();

    // Move-only: simpler than managing copy of the pimpl.
    DatabaseService(DatabaseService&&) noexcept;
    DatabaseService& operator=(DatabaseService&&) noexcept;

    // Disable copy: copying a pimpl is tricky (deep copy vs shared state).
    DatabaseService(const DatabaseService&) = delete;
    DatabaseService& operator=(const DatabaseService&) = delete;

    // Public methods - forward to implementation.
    void execute_query(const std::string& query);
    std::vector<std::string> fetch_results();

   private:
    // Opaque pointer - hides implementation details.
    std::unique_ptr<Impl> pimpl_;
};
