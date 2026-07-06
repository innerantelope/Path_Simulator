# Testing

## Backend

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The tests cover graph storage, JSON persistence, A*, Dijkstra, BFS, city generation, road closures, accidents, vehicle spawning, and simulation controls.

## Frontend

```powershell
cd frontend
npm ci
npm run build
```

The production build performs strict TypeScript checks before bundling.
