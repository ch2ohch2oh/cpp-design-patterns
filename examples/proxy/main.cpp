// Pattern: Proxy
//
// Backend relevance:
// - Proxies wrap a real service client to add cross-cutting behavior:
//   caching, auth, retries, rate limits, tracing, etc.
// - The proxy keeps the same interface, so callers do not need to change.
//
// This example:
// - Defines a feature-flag service interface.
// - Wraps the real service with a caching proxy.

#include <optional>
#include <string>
#include <unordered_map>

#include "../common/log.h"

class FeatureFlagService {
   public:
    virtual ~FeatureFlagService() = default;
    virtual bool is_enabled(std::string flag_name) = 0;
};

class RemoteFeatureFlagService : public FeatureFlagService {
   public:
    bool is_enabled(std::string flag_name) override {
        examples::log_line("remote lookup for flag=", flag_name);
        return flag_name == "new-checkout";
    }
};

class CachingFeatureFlagProxy : public FeatureFlagService {
   public:
    explicit CachingFeatureFlagProxy(FeatureFlagService& inner) : inner_(inner) {}

    bool is_enabled(std::string flag_name) override {
        auto it = cache_.find(flag_name);
        if (it != cache_.end()) {
            examples::log_line("proxy cache hit for flag=", flag_name);
            return it->second;
        }

        bool enabled = inner_.is_enabled(flag_name);
        cache_[flag_name] = enabled;
        return enabled;
    }

   private:
    FeatureFlagService& inner_;
    std::unordered_map<std::string, bool> cache_;
};

static void check_flag(FeatureFlagService& service, const std::string& flag_name) {
    examples::log_line("flag=", flag_name, " enabled=", service.is_enabled(flag_name));
}

int main() {
    examples::log_line("Proxy example: add caching around a remote client.");

    RemoteFeatureFlagService remote;
    CachingFeatureFlagProxy proxy(remote);

    check_flag(proxy, "new-checkout");
    check_flag(proxy, "beta-search");
    check_flag(proxy, "new-checkout");
}
