// Pattern: NVI (Non-Virtual Interface)
//
// Backend relevance:
// - Service components need consistent behavior across implementations (logging, metrics,
// validation).
// - Derived classes might forget to check preconditions or log operations.
// - NVI enforces invariants in the base class by making public methods non-virtual
//   and calling private virtual implementations.
// - Guarantees that locking, logging, and validation always run, regardless of override.
//
// Note: NVI is structurally identical to the Template Method pattern. The difference is
// primarily terminology: NVI is the C++ community's name for this idiom, emphasizing the
// design of the public interface (non-virtual) vs private virtual implementations.
//
// This example:
// - Shows a `CacheService` base class with public non-virtual methods.
// - The public methods enforce invariants (logging, metrics, validation).
// - Derived classes override private virtual methods for actual implementation.
// - Demonstrates that invariants run even when derived classes change.

#include <string>
#include <unordered_map>

#include "../common/log.h"

class CacheService {
   public:
    virtual ~CacheService() = default;

    // Public non-virtual interface - enforces invariants
    void get(const std::string& key) {
        examples::log_line("[CacheService] GET request for key: ", key);
        validate_key(key);
        increment_metrics("get");
        get_impl(key);
    }

    void put(const std::string& key, const std::string& value) {
        examples::log_line("[CacheService] PUT request for key: ", key);
        validate_key(key);
        validate_value(value);
        increment_metrics("put");
        put_impl(key, value);
    }

    void remove(const std::string& key) {
        examples::log_line("[CacheService] REMOVE request for key: ", key);
        validate_key(key);
        increment_metrics("remove");
        remove_impl(key);
    }

   protected:
    // Private virtual implementation - overridden by derived classes
    virtual void get_impl(const std::string& key) = 0;
    virtual void put_impl(const std::string& key, const std::string& value) = 0;
    virtual void remove_impl(const std::string& key) = 0;

   private:
    void validate_key(const std::string& key) {
        if (key.empty()) {
            throw std::invalid_argument("cache key cannot be empty");
        }
        if (key.size() > 256) {
            throw std::invalid_argument("cache key too long (max 256 chars)");
        }
    }

    void validate_value(const std::string& value) {
        if (value.size() > 1024 * 1024) {  // 1MB limit
            throw std::invalid_argument("cache value too large (max 1MB)");
        }
    }

    void increment_metrics(const std::string& operation) {
        // In production, this would increment Prometheus/StatsD metrics
        examples::log_line("[Metrics] cache.", operation, " +1");
    }
};

// In-memory cache implementation
class InMemoryCache : public CacheService {
   public:
    InMemoryCache() : hits_(0), misses_(0) {}

    void get_impl(const std::string& key) override {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            hits_++;
            examples::log_line("[InMemoryCache] HIT: ", key, " = ", it->second);
        } else {
            misses_++;
            examples::log_line("[InMemoryCache] MISS: ", key);
        }
    }

    void put_impl(const std::string& key, const std::string& value) override {
        cache_[key] = value;
        examples::log_line("[InMemoryCache] STORED: ", key, " = ", value);
    }

    void remove_impl(const std::string& key) override {
        size_t erased = cache_.erase(key);
        if (erased > 0) {
            examples::log_line("[InMemoryCache] REMOVED: ", key);
        } else {
            examples::log_line("[InMemoryCache] KEY NOT FOUND: ", key);
        }
    }

    int hits() const { return hits_; }
    int misses() const { return misses_; }

   private:
    std::unordered_map<std::string, std::string> cache_;
    int hits_;
    int misses_;
};

// Redis cache implementation (mocked)
class RedisCache : public CacheService {
   public:
    void get_impl(const std::string& key) override {
        // In production, this would call hiredis/redis-plus-plus
        examples::log_line("[RedisCache] GET ", key, " from Redis cluster");
    }

    void put_impl(const std::string& key, const std::string& value) override {
        examples::log_line("[RedisCache] SET ", key, " = ", value, " in Redis cluster");
    }

    void remove_impl(const std::string& key) override {
        examples::log_line("[RedisCache] DEL ", key, " from Redis cluster");
    }
};

int main() {
    examples::log_line("NVI example: enforcing invariants in cache service components.");

    examples::log_line("\n--- Using InMemoryCache ---");
    InMemoryCache mem_cache;
    mem_cache.put("user:123", "alice");
    mem_cache.put("user:456", "bob");
    mem_cache.get("user:123");
    mem_cache.get("user:999");  // miss
    mem_cache.remove("user:456");

    examples::log_line("\n--- Using RedisCache ---");
    RedisCache redis_cache;
    redis_cache.put("session:abc", "data");
    redis_cache.get("session:abc");
    redis_cache.remove("session:abc");

    examples::log_line("\n--- Invariant enforcement ---");
    examples::log_line("Attempting invalid operations (should throw):");
    try {
        redis_cache.put("", "value");  // empty key
    } catch (const std::exception& e) {
        examples::log_line("  Caught: ", e.what());
    }

    try {
        redis_cache.put("key", std::string(2 * 1024 * 1024, 'x'));  // value too large
    } catch (const std::exception& e) {
        examples::log_line("  Caught: ", e.what());
    }

    examples::log_line(
        "\nNote: Even though derived classes override virtual methods, "
        "the base class ensures logging, validation, and metrics always run.");
}
