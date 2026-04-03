// Pattern: Adapter
//
// Backend relevance:
// - Third-party libraries rarely expose the interface your application wants.
// - An adapter translates between "their API" and "your API" so the rest of the
//   codebase can stay consistent.
//
// This example:
// - Defines an app-level `AlertSink` interface.
// - Adapts a legacy pager client that uses a different method name and data shape.

#include <memory>
#include <string>
#include <string_view>

#include "../common/log.h"

enum class Severity { kInfo, kWarning, kCritical };

class AlertSink {
   public:
    virtual ~AlertSink() = default;
    virtual void send_alert(std::string service, Severity severity, std::string message) = 0;
};

class LegacyPager {
   public:
    void dispatch_page(std::string_view destination, int priority, std::string_view body) {
        examples::log_line("legacy pager => dest=", destination, " priority=", priority,
                           " body=", body);
    }
};

class PagerAdapter : public AlertSink {
   public:
    explicit PagerAdapter(LegacyPager& pager) : pager_(pager) {}

    void send_alert(std::string service, Severity severity, std::string message) override {
        // The adapter translates app concepts into whatever the third-party client expects.
        pager_.dispatch_page(route_for(service), priority_for(severity), message);
    }

   private:
    static std::string route_for(std::string_view service) {
        return std::string("oncall/") + std::string(service);
    }

    static int priority_for(Severity severity) {
        switch (severity) {
            case Severity::kInfo:
                return 1;
            case Severity::kWarning:
                return 3;
            case Severity::kCritical:
                return 5;
        }
        return 1;
    }

    LegacyPager& pager_;
};

static void emit_disk_alert(AlertSink& sink, int free_gb) {
    Severity severity = free_gb < 5 ? Severity::kCritical : Severity::kWarning;
    sink.send_alert("storage", severity, "disk space low: " + std::to_string(free_gb) + "GB");
}

int main() {
    examples::log_line("Adapter example: normalize a legacy client.");

    LegacyPager pager;
    PagerAdapter sink(pager);

    emit_disk_alert(sink, 12);
    emit_disk_alert(sink, 3);
}
