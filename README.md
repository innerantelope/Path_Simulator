# SmartTraffic AI

Real-Time Smart Traffic Simulation powered by the A* Search Algorithm.

SmartTraffic AI is a full-stack traffic simulation platform where vehicles navigate a live city graph while congestion, traffic lights, road closures, accidents, and emergency priority continuously change route costs.

## Highlights

- C++20 backend using Crow, CMake, nlohmann/json, and GoogleTest.
- React + TypeScript frontend using Vite, Tailwind CSS, React Query, React Router, Canvas, Chart.js, and WebSockets.
- A*, Dijkstra, and BFS implemented from scratch for live algorithm comparison.
- Dynamic city generation: grid, random, circular, and radial layouts.
- Vehicle system: cars, buses, police, fire trucks, and ambulances.
- Traffic lights with emergency vehicle priority.
- Continuous congestion model that updates graph edge weights in real time.
- Road closures, accident creation, automatic rerouting, and live analytics.
- Docker, Docker Compose, Render, Vercel, and GitHub Actions configuration.

## Project Structure

```text
backend/
  include/
  src/
  tests/
  CMakeLists.txt
frontend/
  src/
  public/
docs/
docker/
.github/workflows/
docker-compose.yml
render.yaml
vercel.json
```

## Run Locally

### Backend

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
.\build\backend\smarttraffic_server.exe
```

If the host machine does not have a modern C++20 toolchain:

```powershell
docker run --rm -v "${PWD}:/workspace" -w /workspace gcc:13 bash -lc "apt-get update && apt-get install -y cmake git libasio-dev && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel && ctest --test-dir build --output-on-failure"
```

### Frontend

```powershell
cd frontend
npm ci
npm run dev
```

Open `http://localhost:5173`.

### Docker Compose

```powershell
docker compose up --build
```

Frontend: `http://localhost:5173`

Backend: `http://localhost:18080`

## Test

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

```powershell
cd frontend
npm run build
```

## API

The backend exposes REST endpoints for city generation, vehicles, roads, accidents, stats, simulation control, and algorithm comparison. WebSocket updates are broadcast from `/ws` at 20 FPS.

See [docs/API.md](docs/API.md).

## Architecture

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for system, class, and sequence diagrams.

## Deployment

- Backend: Render Docker service using [render.yaml](render.yaml).
- Frontend: Vercel Vite deployment using [vercel.json](vercel.json).
- CI: GitHub Actions in [.github/workflows/ci.yml](.github/workflows/ci.yml).

See [docs/DEPLOYMENT.md](docs/DEPLOYMENT.md).
