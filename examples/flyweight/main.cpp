// Pattern: Flyweight
//
// Backend relevance:
// - Many objects share large portions of their state (e.g., user roles, configs, templates).
// - Duplicating this state wastes memory, especially under high request load.
// - Flyweight shares intrinsic (immutable) state while keeping extrinsic (context) state separate.
// - Common in permission systems, configuration management, message templates.
//
// This example:
// - User roles (admin, editor, viewer) are shared flyweights with intrinsic permission data.
// - Each user context holds extrinsic state (user ID, session info) and references a shared role.
// - Demonstrates memory savings by reusing role objects across many user sessions.

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../common/log.h"

// Intrinsic state: shared, immutable role data
class RoleFlyweight {
   public:
    RoleFlyweight(std::string name, std::unordered_set<std::string> permissions)
        : name_(std::move(name)), permissions_(std::move(permissions)) {}

    const std::string& name() const { return name_; }
    const std::unordered_set<std::string>& permissions() const { return permissions_; }

    bool has_permission(const std::string& perm) const { return permissions_.count(perm) > 0; }

   private:
    std::string name_;
    std::unordered_set<std::string> permissions_;
};

// Flyweight factory: creates and manages shared role objects
class RoleFactory {
   public:
    // Get or create a shared role flyweight
    std::shared_ptr<RoleFlyweight> get_role(const std::string& role_name) {
        auto it = roles_.find(role_name);
        if (it != roles_.end()) {
            examples::log_line("[RoleFactory] Reusing existing role: ", role_name);
            return it->second;
        }

        examples::log_line("[RoleFactory] Creating new role: ", role_name);
        auto role = create_role(role_name);
        roles_[role_name] = role;
        return role;
    }

    size_t shared_role_count() const { return roles_.size(); }

   private:
    std::shared_ptr<RoleFlyweight> create_role(const std::string& role_name) {
        if (role_name == "admin") {
            return std::make_shared<RoleFlyweight>(
                "admin", std::unordered_set<std::string>{"read", "write", "delete", "admin"});
        } else if (role_name == "editor") {
            return std::make_shared<RoleFlyweight>(
                "editor", std::unordered_set<std::string>{"read", "write"});
        } else if (role_name == "viewer") {
            return std::make_shared<RoleFlyweight>("viewer",
                                                   std::unordered_set<std::string>{"read"});
        }
        // Default: no permissions
        return std::make_shared<RoleFlyweight>(role_name, std::unordered_set<std::string>{});
    }

    std::unordered_map<std::string, std::shared_ptr<RoleFlyweight>> roles_;
};

// Extrinsic state: per-user context that references shared role
class UserContext {
   public:
    UserContext(std::string user_id, std::shared_ptr<RoleFlyweight> role)
        : user_id_(std::move(user_id)), role_(std::move(role)) {}

    const std::string& user_id() const { return user_id_; }
    const std::shared_ptr<RoleFlyweight>& role() const { return role_; }

    bool can(const std::string& permission) const {
        bool allowed = role_->has_permission(permission);
        examples::log_line("  User ", user_id_, " (role: ", role_->name(), ") ",
                           allowed ? "ALLOWED" : "DENIED", " for: ", permission);
        return allowed;
    }

   private:
    std::string user_id_;                  // Extrinsic: unique per user
    std::shared_ptr<RoleFlyweight> role_;  // Intrinsic: shared across users
};

int main() {
    examples::log_line("Flyweight example: sharing role data across user contexts.");

    RoleFactory factory;

    // Create multiple user contexts - roles will be shared
    std::vector<UserContext> users;
    users.emplace_back("alice", factory.get_role("admin"));
    users.emplace_back("bob", factory.get_role("editor"));
    users.emplace_back("charlie", factory.get_role("viewer"));
    users.emplace_back("diana", factory.get_role("admin"));   // Reuses admin role
    users.emplace_back("eve", factory.get_role("editor"));    // Reuses editor role
    users.emplace_back("frank", factory.get_role("viewer"));  // Reuses viewer role

    examples::log_line("\nShared role objects created: ", factory.shared_role_count());
    examples::log_line("Total user contexts: ", users.size());
    examples::log_line("Memory saved: ", users.size() - factory.shared_role_count(),
                       " role objects not duplicated\n");

    // Check permissions for each user
    examples::log_line("--- Permission checks ---");
    for (const auto& user : users) {
        examples::log_line("\nUser: ", user.user_id(), " (role: ", user.role()->name(), ")");
        user.can("read");
        user.can("write");
        user.can("delete");
        user.can("admin");
    }

    examples::log_line("\n--- Without flyweight ---");
    examples::log_line("Without sharing, we would need ", users.size(), " separate role objects.");
    examples::log_line("With flyweight, we only need ", factory.shared_role_count(),
                       " shared role objects.");
    examples::log_line("This becomes significant at scale (thousands of users, few roles).");
}
