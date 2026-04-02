# C++ backend patterns — minimal examples (CMake)

This repo contains three tiny, realistic examples that demonstrate:

1. RAII
2. Smart-pointer ownership
3. Rule of 0 / Rule of 5

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
```

## Format

```sh
./scripts/format.sh
```

## Optional: enforce formatting on commit

```sh
git config core.hooksPath .githooks
```
