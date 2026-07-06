import { useEffect, useMemo, useRef, useState } from 'react';
import type { CityResponse, Node, RoadState, SimulationSnapshot, TrafficLight, Vehicle } from '../types';

interface SimulationCanvasProps {
  city?: CityResponse;
  snapshot: SimulationSnapshot | null;
  selectedRoad: RoadState | null;
  onSelectRoad: (road: RoadState | null) => void;
}

interface Point {
  x: number;
  y: number;
}

interface Viewport {
  scale: number;
  offsetX: number;
  offsetY: number;
}

const roadColor = (road: RoadState) => {
  if (road.closed) {
    return '#ff5c7a';
  }
  if (road.accident) {
    return '#f8c14a';
  }
  const score = road.congestionScore;
  if (score > 0.7) {
    return '#ff7a59';
  }
  if (score > 0.35) {
    return '#f8c14a';
  }
  return '#2ddf8f';
};

const vehicleColor = (vehicle: Vehicle) => {
  if (vehicle.type === 'ambulance') return '#ffffff';
  if (vehicle.type === 'fire_truck') return '#ff5c7a';
  if (vehicle.type === 'police') return '#5fb3ff';
  if (vehicle.type === 'bus') return '#f8c14a';
  return '#23c7d9';
};

function boundsFor(nodes: Node[]) {
  if (nodes.length === 0) {
    return { minX: 0, minY: 0, maxX: 1, maxY: 1 };
  }

  return nodes.reduce(
    (bounds, node) => ({
      minX: Math.min(bounds.minX, node.x),
      minY: Math.min(bounds.minY, node.y),
      maxX: Math.max(bounds.maxX, node.x),
      maxY: Math.max(bounds.maxY, node.y),
    }),
    { minX: nodes[0].x, minY: nodes[0].y, maxX: nodes[0].x, maxY: nodes[0].y },
  );
}

function distanceToSegment(point: Point, start: Point, end: Point) {
  const dx = end.x - start.x;
  const dy = end.y - start.y;
  if (dx === 0 && dy === 0) {
    return Math.hypot(point.x - start.x, point.y - start.y);
  }

  const t = Math.max(0, Math.min(1, ((point.x - start.x) * dx + (point.y - start.y) * dy) / (dx * dx + dy * dy)));
  const projected = { x: start.x + t * dx, y: start.y + t * dy };
  return Math.hypot(point.x - projected.x, point.y - projected.y);
}

export function SimulationCanvas({ city, snapshot, selectedRoad, onSelectRoad }: SimulationCanvasProps) {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const containerRef = useRef<HTMLDivElement | null>(null);
  const dragRef = useRef<{ active: boolean; lastX: number; lastY: number; moved: boolean }>({
    active: false,
    lastX: 0,
    lastY: 0,
    moved: false,
  });
  const [size, setSize] = useState({ width: 900, height: 640 });
  const [viewport, setViewport] = useState<Viewport>({ scale: 1, offsetX: 0, offsetY: 0 });

  const nodes = city?.graph.nodes ?? [];
  const roads = snapshot?.roads ?? city?.roads ?? [];
  const trafficLights = snapshot?.trafficLights ?? city?.trafficLights ?? [];
  const vehicles = snapshot?.vehicles ?? [];

  const nodeById = useMemo(() => new Map(nodes.map((node) => [node.id, node])), [nodes]);
  const lightByNode = useMemo(() => new Map(trafficLights.map((light) => [light.nodeId, light])), [trafficLights]);

  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    const observer = new ResizeObserver((entries) => {
      const entry = entries[0];
      setSize({
        width: Math.max(320, Math.floor(entry.contentRect.width)),
        height: Math.max(320, Math.floor(entry.contentRect.height)),
      });
    });

    observer.observe(container);
    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    if (nodes.length === 0) return;

    const bounds = boundsFor(nodes);
    const padding = 80;
    const worldWidth = Math.max(1, bounds.maxX - bounds.minX);
    const worldHeight = Math.max(1, bounds.maxY - bounds.minY);
    const scale = Math.min((size.width - padding) / worldWidth, (size.height - padding) / worldHeight);

    setViewport({
      scale: Math.max(0.2, scale),
      offsetX: size.width / 2 - ((bounds.minX + bounds.maxX) / 2) * scale,
      offsetY: size.height / 2 - ((bounds.minY + bounds.maxY) / 2) * scale,
    });
  }, [nodes, size.width, size.height]);

  const worldToScreen = (point: Point): Point => ({
    x: point.x * viewport.scale + viewport.offsetX,
    y: point.y * viewport.scale + viewport.offsetY,
  });

  const screenToWorld = (point: Point): Point => ({
    x: (point.x - viewport.offsetX) / viewport.scale,
    y: (point.y - viewport.offsetY) / viewport.scale,
  });

  useEffect(() => {
    const canvas = canvasRef.current;
    const context = canvas?.getContext('2d');
    if (!canvas || !context) return;

    const dpr = window.devicePixelRatio || 1;
    canvas.width = Math.floor(size.width * dpr);
    canvas.height = Math.floor(size.height * dpr);
    canvas.style.width = `${size.width}px`;
    canvas.style.height = `${size.height}px`;
    context.setTransform(dpr, 0, 0, dpr, 0, 0);

    context.clearRect(0, 0, size.width, size.height);
    const gradient = context.createLinearGradient(0, 0, size.width, size.height);
    gradient.addColorStop(0, '#071027');
    gradient.addColorStop(1, '#050816');
    context.fillStyle = gradient;
    context.fillRect(0, 0, size.width, size.height);

    context.lineCap = 'round';
    context.lineJoin = 'round';

    for (const road of roads) {
      const source = nodeById.get(road.source);
      const target = nodeById.get(road.target);
      if (!source || !target) continue;

      const start = worldToScreen(source);
      const end = worldToScreen(target);
      const selected = selectedRoad?.source === road.source && selectedRoad?.target === road.target;
      context.strokeStyle = roadColor(road);
      context.globalAlpha = selected ? 1 : 0.72;
      context.lineWidth = selected ? 7 : 3 + road.congestionScore * 5;
      context.beginPath();
      context.moveTo(start.x, start.y);
      context.lineTo(end.x, end.y);
      context.stroke();
    }

    context.globalAlpha = 1;
    for (const node of nodes) {
      const screen = worldToScreen(node);
      const light = lightByNode.get(node.id) as TrafficLight | undefined;
      context.fillStyle = '#101a33';
      context.strokeStyle = '#46547a';
      context.lineWidth = 1;
      context.beginPath();
      context.arc(screen.x, screen.y, 5.5, 0, Math.PI * 2);
      context.fill();
      context.stroke();

      if (light) {
        context.fillStyle = light.state === 'green' ? '#2ddf8f' : light.state === 'yellow' ? '#f8c14a' : '#ff5c7a';
        context.beginPath();
        context.arc(screen.x + 7, screen.y - 7, 3.5, 0, Math.PI * 2);
        context.fill();
      }
    }

    for (const vehicle of vehicles) {
      const screen = worldToScreen(vehicle.position);
      const radius = vehicle.emergency ? 7 : 5;
      if (vehicle.emergency) {
        context.strokeStyle = vehicleColor(vehicle);
        context.globalAlpha = 0.25 + 0.2 * Math.sin(Date.now() / 120);
        context.lineWidth = 3;
        context.beginPath();
        context.arc(screen.x, screen.y, 13, 0, Math.PI * 2);
        context.stroke();
        context.globalAlpha = 1;
      }

      context.fillStyle = vehicleColor(vehicle);
      context.strokeStyle = '#050816';
      context.lineWidth = 2;
      context.beginPath();
      context.arc(screen.x, screen.y, radius, 0, Math.PI * 2);
      context.fill();
      context.stroke();
    }
  }, [nodes, roads, vehicles, trafficLights, selectedRoad, size, viewport, nodeById, lightByNode]);

  const selectRoadAt = (clientX: number, clientY: number) => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const worldPoint = screenToWorld({ x: clientX - rect.left, y: clientY - rect.top });
    let nearest: RoadState | null = null;
    let nearestDistance = Number.POSITIVE_INFINITY;

    for (const road of roads) {
      const source = nodeById.get(road.source);
      const target = nodeById.get(road.target);
      if (!source || !target) continue;

      const distance = distanceToSegment(worldPoint, source, target);
      if (distance < nearestDistance) {
        nearestDistance = distance;
        nearest = road;
      }
    }

    onSelectRoad(nearestDistance * viewport.scale <= 14 ? nearest : null);
  };

  return (
    <div ref={containerRef} className="relative h-full min-h-[420px] overflow-hidden rounded-lg border border-line bg-ink shadow-glow">
      <canvas
        ref={canvasRef}
        className="block cursor-grab active:cursor-grabbing"
        onWheel={(event) => {
          event.preventDefault();
          const delta = event.deltaY > 0 ? 0.9 : 1.1;
          const rect = event.currentTarget.getBoundingClientRect();
          const cursor = { x: event.clientX - rect.left, y: event.clientY - rect.top };
          const before = screenToWorld(cursor);
          const nextScale = Math.max(0.15, Math.min(8, viewport.scale * delta));
          setViewport({
            scale: nextScale,
            offsetX: cursor.x - before.x * nextScale,
            offsetY: cursor.y - before.y * nextScale,
          });
        }}
        onPointerDown={(event) => {
          event.currentTarget.setPointerCapture(event.pointerId);
          dragRef.current = { active: true, lastX: event.clientX, lastY: event.clientY, moved: false };
        }}
        onPointerMove={(event) => {
          const drag = dragRef.current;
          if (!drag.active) return;
          const dx = event.clientX - drag.lastX;
          const dy = event.clientY - drag.lastY;
          if (Math.abs(dx) + Math.abs(dy) > 2) {
            drag.moved = true;
          }
          drag.lastX = event.clientX;
          drag.lastY = event.clientY;
          setViewport((current) => ({
            ...current,
            offsetX: current.offsetX + dx,
            offsetY: current.offsetY + dy,
          }));
        }}
        onPointerUp={(event) => {
          const drag = dragRef.current;
          dragRef.current.active = false;
          if (!drag.moved) {
            selectRoadAt(event.clientX, event.clientY);
          }
        }}
      />
      <div className="pointer-events-none absolute left-4 top-4 rounded-md border border-line bg-panel/90 px-3 py-2 text-xs text-slate-300">
        <div className="font-semibold text-white">Live City Canvas</div>
        <div>{nodes.length.toLocaleString()} intersections · {roads.length.toLocaleString()} roads · {vehicles.length.toLocaleString()} vehicles</div>
      </div>
      {selectedRoad && (
        <div className="pointer-events-none absolute bottom-4 left-4 rounded-md border border-line bg-panel/95 px-3 py-2 text-xs text-slate-200">
          <div className="font-semibold text-white">
            {selectedRoad.source} to {selectedRoad.target}
          </div>
          <div>
            congestion {(selectedRoad.congestionScore * 100).toFixed(0)}% · vehicles {selectedRoad.vehicleCount} · cost{' '}
            {selectedRoad.currentWeight.toFixed(1)}
          </div>
        </div>
      )}
    </div>
  );
}
