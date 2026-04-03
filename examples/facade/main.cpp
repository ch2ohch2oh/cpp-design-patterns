// Pattern: Facade
//
// Backend relevance:
// - Backend request handlers often need to touch several subsystems together:
//   cache, database, metrics, tracing, etc.
// - A facade gives callers one small API while hiding the orchestration details.
//
// This example:
// - Exposes `UserProfileFacade::fetch_profiles()` as the main API.
// - Uses a cache and database that both support true batch lookups.
// - Keeps the orchestration logic in one place: cache batch, DB batch fallback,
//   cache warm-up, and metrics.

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../common/log.h"

struct UserProfile {
    int user_id{0};
    std::string name;
    std::string tier;
};

class Metrics {
   public:
    void record_cache_hit() { ++cache_hits_; }
    void record_db_hit() { ++db_hits_; }
    void record_miss() { ++misses_; }

    void dump() const {
        examples::log_line("metrics: cache_hits=", cache_hits_, " db_hits=", db_hits_,
                           " misses=", misses_);
    }

   private:
    int cache_hits_{0};
    int db_hits_{0};
    int misses_{0};
};

class ProfileCache {
   public:
    std::vector<std::optional<UserProfile>> get_many(const std::vector<int>& user_ids) const {
        examples::log_line("cache.get_many(", user_ids.size(), " ids)");

        std::vector<std::optional<UserProfile>> results;
        results.reserve(user_ids.size());
        for (int user_id : user_ids) {
            auto it = cache_.find(user_id);
            if (it == cache_.end()) {
                results.push_back(std::nullopt);
                continue;
            }
            results.push_back(it->second);
        }
        return results;
    }

    void put_many(const std::vector<UserProfile>& profiles) {
        for (const UserProfile& profile : profiles) {
            cache_[profile.user_id] = profile;
        }
    }

   private:
    std::unordered_map<int, UserProfile> cache_{
        {7, UserProfile{.user_id = 7, .name = "Grace", .tier = "gold"}},
    };
};

class ProfileDatabase {
   public:
    std::vector<std::optional<UserProfile>> load_many(const std::vector<int>& user_ids) const {
        examples::log_line("db.load_many(", user_ids.size(), " ids)");

        std::vector<std::optional<UserProfile>> results;
        results.reserve(user_ids.size());
        for (int user_id : user_ids) {
            auto it = rows_.find(user_id);
            if (it == rows_.end()) {
                results.push_back(std::nullopt);
                continue;
            }
            results.push_back(it->second);
        }
        return results;
    }

   private:
    std::unordered_map<int, UserProfile> rows_{
        {42, UserProfile{.user_id = 42, .name = "Ada", .tier = "platinum"}},
        {7, UserProfile{.user_id = 7, .name = "Grace", .tier = "gold"}},
        {8, UserProfile{.user_id = 8, .name = "Linus", .tier = "silver"}},
    };
};

class UserProfileFacade {
   public:
    UserProfileFacade(ProfileCache& cache, ProfileDatabase& db, Metrics& metrics)
        : cache_(cache), db_(db), metrics_(metrics) {}

    std::vector<std::optional<UserProfile>> fetch_profiles(const std::vector<int>& user_ids) {
        std::vector<std::optional<UserProfile>> results = cache_.get_many(user_ids);

        std::vector<int> missed_ids;
        std::vector<std::size_t> missed_positions;
        for (std::size_t i = 0; i < results.size(); ++i) {
            if (results[i]) {
                metrics_.record_cache_hit();
                continue;
            }
            missed_ids.push_back(user_ids[i]);
            missed_positions.push_back(i);
        }

        if (missed_ids.empty()) {
            return results;
        }

        std::vector<std::optional<UserProfile>> loaded = db_.load_many(missed_ids);
        std::vector<UserProfile> profiles_to_cache;

        // The facade merges DB answers back into the original request order so callers
        // get one result vector aligned with the input IDs.
        for (std::size_t i = 0; i < loaded.size(); ++i) {
            const std::size_t pos = missed_positions[i];
            if (loaded[i]) {
                metrics_.record_db_hit();
                results[pos] = loaded[i];
                profiles_to_cache.push_back(*loaded[i]);
            } else {
                metrics_.record_miss();
            }
        }

        if (!profiles_to_cache.empty()) {
            // Cache warm-up is also batched instead of issuing one write per profile.
            cache_.put_many(profiles_to_cache);
        }

        return results;
    }

   private:
    ProfileCache& cache_;
    ProfileDatabase& db_;
    Metrics& metrics_;
};

static void print_profiles(const std::vector<int>& user_ids,
                           const std::vector<std::optional<UserProfile>>& profiles) {
    for (std::size_t i = 0; i < user_ids.size(); ++i) {
        if (profiles[i]) {
            examples::log_line("user=", user_ids[i], " name=", profiles[i]->name,
                               " tier=", profiles[i]->tier);
            continue;
        }
        examples::log_line("user=", user_ids[i], " not found");
    }
}

int main() {
    examples::log_line("Facade example: one batch API over cache + DB + metrics.");

    ProfileCache cache;
    ProfileDatabase db;
    Metrics metrics;
    UserProfileFacade facade(cache, db, metrics);

    const std::vector<int> batch_ids{7, 42, 99, 8, 42};

    examples::log_line("first batch:");
    print_profiles(batch_ids, facade.fetch_profiles(batch_ids));

    examples::log_line("second batch:");
    print_profiles(batch_ids, facade.fetch_profiles(batch_ids));

    metrics.dump();
}
