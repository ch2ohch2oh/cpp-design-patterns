# Design patterns every backend C++ programmer should know (ranked)

| Rank | Pattern / Idiom | Category | Backend C++ focus |
|---:|---|---|---|
| 1 | RAII | C++ idiom | Leak-free sockets/files/locks; exception-safe cleanup. |
| 2 | Ownership with smart pointers | C++ idiom | Makes lifetimes explicit across async + callbacks. |
| 3 | Rule of 0 / Rule of 5 | C++ idiom | Prevents accidental copies/moves in request/response objects. |
| 4 | Thread pool / work queue | Concurrency | Core throughput primitive for RPC/HTTP handlers and background work. |
| 5 | Producer–consumer (bounded queue) | Concurrency | Backpressure; protects latency under load. |
| 6 | Futures/promises (task-based async) | Concurrency | Composable async workflows; cancellation/timeout integration. |
| 7 | Reactor (event loop) | Concurrency/IO | Scalable network IO (epoll/kqueue-style architectures). |
| 8 | Facade | GoF (Structural) | Wraps subsystems (DB, cache, metrics) behind clean APIs. |
| 9 | Adapter | GoF (Structural) | Normalizes 3rd-party clients/libs into your interfaces. |
| 10 | Strategy | GoF (Behavioral) | Swappable policies (retry, routing, load balancing, parsing). |
| 11 | Factory Method | GoF (Creational) | Pluggable creation for clients/handlers; test-friendly DI. |
| 12 | Abstract Factory | GoF (Creational) | Environment-based wiring (prod vs test vs staging stacks). |
| 13 | Proxy | GoF (Structural) | Client wrappers for caching, auth, rate limits, retries, tracing. |
| 14 | Decorator | GoF (Structural) | Layer cross-cutting concerns around handlers/clients. |
| 15 | Type erasure | C++ idiom | Store heterogeneous callables/handlers without template bloat. |
| 16 | Observer (pub/sub) | GoF (Behavioral) | Metrics/logging hooks and internal eventing (mind lifetimes). |
| 17 | State | GoF (Behavioral) | Connection/session/protocol state machines without giant `switch`. |
| 18 | Command | GoF (Behavioral) | Queueable jobs; background tasks; audit/redo semantics. |
| 19 | Pimpl | C++ idiom | Cuts rebuild times; hides heavy deps in service libraries. |
| 20 | Bridge | GoF (Structural) | Decouple interface from platform/impl (TLS, DNS, IO backends). |
| 21 | Composite | GoF (Structural) | Middleware chains / routing trees / filter graphs. |
| 22 | NVI (Non-Virtual Interface) | C++ idiom | Enforce invariants in extensible service components. |
| 23 | Traits/tag dispatch/(concepts-style selection) | C++ idiom | Efficient generic code in serialization, hashing, parsing. |
| 24 | Visitor (often via `std::variant` + `std::visit`) | GoF (Behavioral) | Clean handling of protocol/message variants. |
| 25 | Flyweight | GoF (Structural) | Interning/shared immutable data to reduce memory in hot paths. |

