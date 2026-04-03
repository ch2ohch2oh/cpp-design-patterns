# C++ backend patterns — minimal examples (CMake)

This repo contains tiny, realistic examples that demonstrate:

1. RAII
2. Smart-pointer ownership
3. Rule of 0 / Rule of 5
4. Thread pool / work queue
5. Producer–consumer (bounded queue)
6. Futures/promises (task-based async)
7. Reactor (event loop)
8. Facade
9. Adapter
10. Strategy

## Build

```sh
cmake -S . -B build
cmake --build build -j
```

## Run

```sh
./build/examples/raii/raii_example
./build/examples/smart_pointers/smart_pointers_example
./build/examples/rule_of_0_5/rule_of_0_5_example
./build/examples/thread_pool/thread_pool_example
./build/examples/bounded_queue/bounded_queue_example
./build/examples/futures_promises/futures_promises_example
./build/examples/reactor/reactor_example
./build/examples/facade/facade_example
./build/examples/adapter/adapter_example
./build/examples/strategy/strategy_example
```

## Format

```sh
./scripts/format.sh
```

## Optional: enforce formatting on commit

```sh
git config core.hooksPath .githooks
```
