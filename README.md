# C++ backend patterns — minimal examples (CMake, C++20)

Tiny, realistic C++20 examples of common backend patterns and design patterns, each in its own folder under `examples/`.

## Requirements

- CMake 3.20+
- A C++20 compiler (Clang/GCC/MSVC)
- Optional: `clang-format` (for `./scripts/format.sh`)

## Patterns (ranked backend focus)

| Done | Rank | Pattern / Idiom | Category | Backend C++ focus |
|:--:|---:|---|---|---|
| [x] | 1 | [RAII](examples/raii) | C++ idiom | Leak-free sockets/files/locks; exception-safe cleanup. |
| [x] | 2 | [Ownership with smart pointers](examples/smart_pointers) | C++ idiom | Makes lifetimes explicit across async + callbacks. |
| [x] | 3 | [Rule of 0 / Rule of 5](examples/rule_of_0_5) | C++ idiom | Prevents accidental copies/moves in request/response objects. |
| [x] | 4 | [Thread pool / work queue](examples/thread_pool) | Concurrency | Core throughput primitive for RPC/HTTP handlers and background work. |
| [x] | 5 | [Producer–consumer (bounded queue)](examples/bounded_queue) | Concurrency | Backpressure; protects latency under load. |
| [x] | 6 | [Futures/promises (task-based async)](examples/futures_promises) | Concurrency | Composable async workflows; cancellation/timeout integration. |
| [x] | 7 | [Reactor (event loop)](examples/reactor) | Concurrency/IO | Scalable network IO (epoll/kqueue-style architectures). |
| [x] | 8 | [Facade](examples/facade) | GoF (Structural) | Wraps subsystems (DB, cache, metrics) behind clean APIs. |
| [x] | 9 | [Adapter](examples/adapter) | GoF (Structural) | Normalizes 3rd-party clients/libs into your interfaces. |
| [x] | 10 | [Strategy](examples/strategy) | GoF (Behavioral) | Swappable policies (retry, routing, load balancing, parsing). |
| [x] | 11 | [Factory Method](examples/factory_method) | GoF (Creational) | Pluggable creation for clients/handlers; test-friendly DI. |
| [x] | 12 | [Abstract Factory](examples/abstract_factory) | GoF (Creational) | Environment-based wiring (prod vs test vs staging stacks). |
| [x] | 13 | [Proxy](examples/proxy) | GoF (Structural) | Client wrappers for caching, auth, rate limits, retries, tracing. |
| [x] | 14 | [Decorator](examples/decorator) | GoF (Structural) | Layer cross-cutting concerns around handlers/clients. |
| [x] | 15 | [Type erasure](examples/type_erasure) | C++ idiom | Store heterogeneous callables/handlers without template bloat. |
| [x] | 16 | [Observer (pub/sub)](examples/observer) | GoF (Behavioral) | Metrics/logging hooks and internal eventing (mind lifetimes). |
| [x] | 17 | [State](examples/state) | GoF (Behavioral) | Connection/session/protocol state machines without giant `switch`. |
| [x] | 18 | [Command](examples/command) | GoF (Behavioral) | Queueable jobs; background tasks; audit/redo semantics. |
| [x] | 19 | [Pimpl](examples/pimpl) | C++ idiom | Cuts rebuild times; hides heavy deps in service libraries. |
| [x] | 20 | [Bridge](examples/bridge) | GoF (Structural) | Decouple interface from platform/impl (TLS, DNS, IO backends). |
| [x] | 21 | [Composite](examples/composite) | GoF (Structural) | Middleware chains / routing trees / filter graphs. |
| [ ] | 22 | NVI (Non-Virtual Interface) | C++ idiom | Enforce invariants in extensible service components. |
| [x] | 23 | [Traits/tag dispatch/(concepts-style selection)](examples/traits_tag_dispatch) | C++ idiom | Efficient generic code in serialization, hashing, parsing. |
| [x] | 24 | [Visitor](examples/visitor) (often via `std::variant` + `std::visit`) | GoF (Behavioral) | Clean handling of protocol/message variants. |
| [ ] | 25 | Flyweight | GoF (Structural) | Interning/shared immutable data to reduce memory in hot paths. |

## Build

```sh
cmake -S . -B build
cmake --build build -j
```

## Run

```sh
# Run one example:
./build/examples/<pattern>/<pattern>_example

# List built example binaries:
ls -1 ./build/examples/*/*_example
```

## Format

```sh
./scripts/format.sh
```

## Optional: enforce formatting on commit (git hook)

```sh
git config core.hooksPath .githooks
```

## Makefile shortcuts

```sh
make build
make run-raii
make format
```
