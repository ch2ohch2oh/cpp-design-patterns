// Pattern: Observer (pub/sub)
//
// Backend relevance:
// - Decouples event production from side effects.
// - This version uses std::any for a simpler, scalable Dispatcher.
// - Avoids complex variadic templates while remaining boilerplate-free.

#include <any>
#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "../common/log.h"

// --- Events ---

struct OrderPlacedEvent {
    int order_id;
    std::string customer;
};

struct OrderCancelledEvent {
    int order_id;
    std::string reason;
};

// --- Infrastructure ---

// Scalable Dispatcher: Uses std::any to store different handler vectors.
// This is "less templates" and more "standard C++" than variadic approaches.
class Dispatcher {
   public:
    template <typename Event>
    void subscribe(std::function<void(const Event&)> handler) {
        get_handlers<Event>().push_back(std::move(handler));
    }

    template <typename Event>
    void publish(const Event& event) {
        for (const auto& h : get_handlers<Event>()) {
            h(event);
        }
    }

   private:
    std::unordered_map<std::type_index, std::any> handlers_map_;

    // Helper to get or create the handlers vector for a specific type.
    template <typename Event>
    std::vector<std::function<void(const Event&)>>& get_handlers() {
        auto tid = std::type_index(typeid(Event));
        if (handlers_map_.find(tid) == handlers_map_.end()) {
            handlers_map_[tid] = std::vector<std::function<void(const Event&)>>{};
        }
        return std::any_cast<std::vector<std::function<void(const Event&)>>&>(handlers_map_[tid]);
    }
};

// --- Application Logic ---

class OrderService {
   public:
    explicit OrderService(Dispatcher& dispatcher) : dispatcher_(dispatcher) {}

    void place_order(std::string customer) {
        int id = next_order_id_++;
        examples::log_line("order service: placed order=", id);
        dispatcher_.publish(OrderPlacedEvent{.order_id = id, .customer = std::move(customer)});
    }

    void cancel_order(int order_id, std::string reason) {
        examples::log_line("order service: cancelling order=", order_id);
        dispatcher_.publish(OrderCancelledEvent{.order_id = order_id, .reason = std::move(reason)});
    }

   private:
    Dispatcher& dispatcher_;
    int next_order_id_{1001};
};

int main() {
    examples::log_line("Observer example: Simple Scalable Dispatcher (std::any).");

    Dispatcher dispatcher;
    OrderService service(dispatcher);

    dispatcher.subscribe<OrderPlacedEvent>(
        [](const auto& event) { examples::log_line("  [placed] metrics: orders_total += 1"); });

    dispatcher.subscribe<OrderCancelledEvent>([](const auto& event) {
        examples::log_line("  [cancelled] inventory: returning items for order=", event.order_id);
    });

    service.place_order("alice");
    service.cancel_order(1001, "customer changed mind");
}
