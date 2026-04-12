// Pattern: Pimpl (Pointer to Implementation)
//
// Backend relevance:
// - Service libraries often have heavy dependencies (DB drivers, TLS libraries, etc.)
// - Exposing these in headers forces all downstream code to recompile when they change.
// - Pimpl hides implementation behind an opaque pointer, cutting rebuild times.
// - Also hides private members from the public API, improving encapsulation.
//
// This example:
// - Shows a `DatabaseService` class with a Pimpl.
// - The header (database_service.h) only declares a forward-declared `Impl` struct.
// - The implementation (database_service.cpp) contains the heavy deps.
// - Changing `Impl` doesn't require recompiling users of `DatabaseService`.

#include "../common/log.h"
#include "database_service.h"

int main() {
    examples::log_line("Pimpl example: hiding implementation details.");

    DatabaseService db("postgresql://localhost:5432/mydb");

    db.execute_query("SELECT * FROM users");
    db.execute_query("SELECT COUNT(*) FROM orders");

    auto results = db.fetch_results();
    for (const auto& result : results) {
        examples::log_line("  result: ", result);
    }

    examples::log_line(
        "Note: changing Impl's internals doesn't require recompiling this "
        "main.cpp.");
}
