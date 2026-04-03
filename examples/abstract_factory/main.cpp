// Pattern: Abstract Factory
//
// Backend relevance:
// - Different environments often need matching families of objects:
//   real DB + real cache in production, fake DB + fake cache in tests.
// - Abstract Factory creates those related objects as one coherent bundle.
//
// This example:
// - Builds a `UserRepository` from an environment-specific factory.
// - Each factory provides a matching database client and cache client.
//
// Class diagram (simplified):
//
//   BackendFactory <------------------- ProductionBackendFactory
//        |                                     |
//        |                                     +--> ProductionDatabaseClient : DatabaseClient
//        |                                     |
//        |                                     +--> ProductionCacheClient    : CacheClient
//        |
//        +------------------------------- TestBackendFactory
//                                              |
//                                              +--> TestDatabaseClient : DatabaseClient
//                                              |
//                                              +--> TestCacheClient    : CacheClient
//
//   UserRepository
//        |
//        +--> holds std::unique_ptr<DatabaseClient>
//        |
//        +--> holds std::unique_ptr<CacheClient>
//
// The key idea is that one factory creates a matching family of related objects.

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "../common/log.h"

class DatabaseClient {
   public:
    virtual ~DatabaseClient() = default;
    virtual std::optional<std::string> load_user(int user_id) = 0;
};

class CacheClient {
   public:
    virtual ~CacheClient() = default;
    virtual std::optional<std::string> get(int user_id) = 0;
    virtual void put(int user_id, std::string name) = 0;
};

class ProductionDatabaseClient : public DatabaseClient {
   public:
    std::optional<std::string> load_user(int user_id) override {
        return user_id == 42 ? std::optional<std::string>{"Ada"} : std::nullopt;
    }
};

class ProductionCacheClient : public CacheClient {
   public:
    std::optional<std::string> get(int user_id) override {
        auto it = data_.find(user_id);
        if (it == data_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void put(int user_id, std::string name) override {
        examples::log_line("prod cache put user=", user_id, " name=", name);
        data_[user_id] = std::move(name);
    }

   private:
    std::unordered_map<int, std::string> data_{
        {7, "Grace"},
    };
};

class TestDatabaseClient : public DatabaseClient {
   public:
    std::optional<std::string> load_user(int user_id) override {
        return user_id == 5 ? std::optional<std::string>{"test-db-user-5"} : std::nullopt;
    }
};

class TestCacheClient : public CacheClient {
   public:
    std::optional<std::string> get(int user_id) override {
        auto it = data_.find(user_id);
        if (it == data_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void put(int user_id, std::string name) override {
        examples::log_line("test cache put user=", user_id, " name=", name);
        data_[user_id] = std::move(name);
    }

   private:
    std::unordered_map<int, std::string> data_{
        {1, "cached-test-user"},
    };
};

class BackendFactory {
   public:
    virtual ~BackendFactory() = default;
    virtual std::unique_ptr<DatabaseClient> create_database() = 0;
    virtual std::unique_ptr<CacheClient> create_cache() = 0;
};

class ProductionBackendFactory : public BackendFactory {
   public:
    std::unique_ptr<DatabaseClient> create_database() override {
        return std::make_unique<ProductionDatabaseClient>();
    }

    std::unique_ptr<CacheClient> create_cache() override {
        return std::make_unique<ProductionCacheClient>();
    }
};

class TestBackendFactory : public BackendFactory {
   public:
    std::unique_ptr<DatabaseClient> create_database() override {
        return std::make_unique<TestDatabaseClient>();
    }

    std::unique_ptr<CacheClient> create_cache() override {
        return std::make_unique<TestCacheClient>();
    }
};

class UserRepository {
   public:
    explicit UserRepository(BackendFactory& factory)
        : db_(factory.create_database()), cache_(factory.create_cache()) {}

    std::optional<std::string> get_user_name(int user_id) {
        if (auto cached = cache_->get(user_id)) {
            examples::log_line("cache hit for user=", user_id);
            return cached;
        }
        examples::log_line("cache miss for user=", user_id);
        if (auto loaded = db_->load_user(user_id)) {
            cache_->put(user_id, *loaded);
            return loaded;
        }
        return std::nullopt;
    }

   private:
    std::unique_ptr<DatabaseClient> db_;
    std::unique_ptr<CacheClient> cache_;
};

static void run_demo(const std::string& label, BackendFactory& factory, int user_id) {
    examples::log_line(label);
    UserRepository repo(factory);

    // We query the same repository twice so the example demonstrates that the
    // factory created a matching family of objects that can work together.
    for (int attempt = 1; attempt <= 2; ++attempt) {
        examples::log_line("lookup ", attempt, ":");
        if (auto user = repo.get_user_name(user_id)) {
            examples::log_line("user=", user_id, " name=", *user);
            continue;
        }
        examples::log_line("user=", user_id, " not found");
    }
}

int main() {
    examples::log_line("Abstract Factory example: environment-specific wiring.");

    ProductionBackendFactory prod_factory;
    TestBackendFactory test_factory;

    run_demo("production:", prod_factory, 42);
    run_demo("test:", test_factory, 5);
}
