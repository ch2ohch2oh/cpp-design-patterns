// Pattern: Factory Method
//
// Backend relevance:
// - Backends often need pluggable client creation for prod vs mock vs specialized clients.
// - Factory Method moves creation behind a virtual hook while keeping shared workflow in one place.
//
// This example:
// - Models a service that imports user data from an upstream provider.
// - The base class owns the import workflow.
// - Subclasses decide which client implementation gets created.
//
// Important distinction:
// - A class merely having a `create()` function does NOT automatically mean
//   you are using the Factory Method pattern.
// - It becomes Factory Method when a base class defines a workflow and delegates
//   one creation step to an overridable method implemented by subclasses.

#include <memory>
#include <string>
#include <vector>

#include "../common/log.h"

class UserDirectoryClient {
   public:
    virtual ~UserDirectoryClient() = default;
    virtual std::vector<std::string> fetch_users() = 0;
};

class HttpUserDirectoryClient : public UserDirectoryClient {
   public:
    std::vector<std::string> fetch_users() override { return {"Ada", "Grace", "Linus"}; }
};

class StubUserDirectoryClient : public UserDirectoryClient {
   public:
    std::vector<std::string> fetch_users() override { return {"TestUser1", "TestUser2"}; }
};

class UserImportJob {
   public:
    virtual ~UserImportJob() = default;

    void run() {
        // `run()` is the fixed algorithm:
        // 1. create a client
        // 2. fetch users through that client
        // 3. process the results
        //
        // This looks like Template Method structurally: a base class defines the
        // algorithm and subclasses fill in one step. It is specifically Factory
        // Method because the overridable step is object creation.
        //
        // The base class owns this workflow. It does NOT know which concrete
        // client type it will get back.
        std::unique_ptr<UserDirectoryClient> client = create_client();
        std::vector<std::string> users = client->fetch_users();

        examples::log_line("imported ", users.size(), " users");
        for (const std::string& user : users) {
            examples::log_line("user=", user);
        }
    }

   protected:
    // This is the actual factory method in the pattern:
    // - the base class calls it from `run()`
    // - subclasses override it to choose the concrete product
    //
    // If this were just a standalone `create_client()` helper on some unrelated
    // class, that would be "a factory", but not necessarily the Factory Method pattern.
    virtual std::unique_ptr<UserDirectoryClient> create_client() = 0;
};

class ProductionUserImportJob : public UserImportJob {
   protected:
    std::unique_ptr<UserDirectoryClient> create_client() override {
        return std::make_unique<HttpUserDirectoryClient>();
    }
};

class TestUserImportJob : public UserImportJob {
   protected:
    std::unique_ptr<UserDirectoryClient> create_client() override {
        return std::make_unique<StubUserDirectoryClient>();
    }
};

int main() {
    examples::log_line("Factory Method example: pluggable client creation.");

    ProductionUserImportJob prod_job;
    TestUserImportJob test_job;

    examples::log_line("production run:");
    prod_job.run();

    examples::log_line("test run:");
    test_job.run();
}
