# SmartTraffic AI

Real-Time Smart Traffic Simulation powered by the A* Search Algorithm.

## Phase 1: Graph Library

The current implementation provides the backend graph foundation:

- `Node`, `Edge`, and `Graph` domain classes.
- Adjacency-list storage.
- Weighted directed and undirected edges.
- Dynamic edge insertion, removal, and weight updates.
- Deterministic JSON serialization.
- JSON load/save support for city graph files.
- GoogleTest coverage for graph behavior and persistence.

## Build and Test

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

On systems without a local C++20/CMake toolchain, the project can be tested with Docker:

```powershell
docker run --rm -v ${PWD}:/workspace -w /workspace gcc:13 bash -lc "apt-get update && apt-get install -y cmake git && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel && ctest --test-dir build --output-on-failure"
```
