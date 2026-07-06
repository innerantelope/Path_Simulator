# SmartTraffic AI API

Base URL: `http://localhost:18080`

## REST

| Method | Path | Description |
| --- | --- | --- |
| `GET` | `/health` | Service health. |
| `GET` | `/city` | Current graph, road states, and traffic lights. |
| `POST` | `/city/random` | Generate a city. Body: `layout`, `intersections`, `roadDensity`, `averageRoadLength`, `directed`, `seed`, `vehicles`. |
| `GET` | `/vehicles` | List vehicles. |
| `POST` | `/vehicles` | Spawn vehicles. Body: `count`, `type`, optional `source`, `destination`, `speed`. |
| `DELETE` | `/vehicles/{id}` | Remove a vehicle. |
| `POST` | `/road/close` | Close a road. Body: `source`, `target`. |
| `POST` | `/road/open` | Reopen a road. Body: `source`, `target`. |
| `POST` | `/accident` | Create an accident. Body: optional `source`, `target`, `kind`, `severity`. |
| `DELETE` | `/accident` | Clear accidents, or delete one with body `id`. |
| `GET` | `/stats` | Live analytics. |
| `GET` | `/simulation` | Simulation state. |
| `POST` | `/simulation/start` | Start ticking. |
| `POST` | `/simulation/pause` | Pause ticking. |
| `POST` | `/simulation/resume` | Resume ticking. |
| `POST` | `/simulation/reset` | Reset city state and vehicles. |
| `POST` | `/simulation/speed` | Body: `speed` as `1`, `2`, `5`, or `10`. |
| `GET` | `/route?source=N0&destination=N8&algorithm=A*` | Run one pathfinding algorithm. |
| `GET` | `/algorithms/compare?source=N0&destination=N8` | Compare A*, Dijkstra, and BFS. |

## WebSocket

Path: `/ws`

The server broadcasts at 20 FPS. Each message contains:

```json
{
  "type": "simulation_update",
  "simulation": {},
  "vehicles": [],
  "trafficLights": [],
  "roads": [],
  "accidents": [],
  "stats": {}
}
```
