# Deployment Guide

## Local Docker

```powershell
docker compose up --build
```

Frontend: `http://localhost:5173`

Backend: `http://localhost:18080`

## Backend on Render

Use `render.yaml` or create a Docker web service manually:

- Dockerfile: `backend/Dockerfile`
- Port env var: `PORT=18080`
- Health check: `/health`

## Frontend on Vercel

Set project root to the repository root. `vercel.json` builds the Vite app from `frontend/`.

Environment variables:

```text
VITE_API_BASE_URL=https://your-render-service.onrender.com
VITE_WS_BASE_URL=wss://your-render-service.onrender.com
```

## Production Notes

- The backend keeps simulation state in memory.
- Scale one backend instance per live simulation room.
- For multi-room deployments, add a room id to REST/WebSocket routes and persist snapshots externally.
