import type {
  AlgorithmComparison,
  CityLayout,
  CityResponse,
  SimulationState,
  SimulationStats,
  Vehicle,
  VehicleType,
} from '../types';

const configuredApiUrl = import.meta.env.VITE_API_URL as string | undefined;
export const API_URL = (configuredApiUrl?.replace(/\/$/, '') || 'http://localhost:18080');

async function request<T>(path: string, options: RequestInit = {}): Promise<T> {
  const response = await fetch(`${API_URL}${path}`, {
    ...options,
    headers: {
      'Content-Type': 'application/json',
      ...(options.headers ?? {}),
    },
  });

  const text = await response.text();
  const payload = text ? JSON.parse(text) : {};
  if (!response.ok) {
    throw new Error(payload.error ?? `Request failed with ${response.status}`);
  }
  return payload as T;
}

export interface GenerateCityPayload {
  layout: CityLayout;
  intersections: number;
  roadDensity: number;
  averageRoadLength: number;
  directed: boolean;
  seed: number;
  vehicles: number;
}

export const api = {
  getCity: () => request<CityResponse>('/city'),
  generateCity: (payload: GenerateCityPayload) =>
    request<{ city: CityResponse; vehicles: Vehicle[] }>('/city/random', {
      method: 'POST',
      body: JSON.stringify(payload),
    }),
  getVehicles: () => request<Vehicle[]>('/vehicles'),
  spawnVehicles: (payload: { count: number; type: VehicleType }) =>
    request<{ ids: string[]; vehicles: Vehicle[] }>('/vehicles', {
      method: 'POST',
      body: JSON.stringify(payload),
    }),
  deleteVehicle: (id: string) => request<{ deleted: string }>(`/vehicles/${id}`, { method: 'DELETE' }),
  closeRoad: (source: string, target: string) =>
    request('/road/close', { method: 'POST', body: JSON.stringify({ source, target }) }),
  openRoad: (source: string, target: string) =>
    request('/road/open', { method: 'POST', body: JSON.stringify({ source, target }) }),
  createAccident: (source?: string, target?: string) =>
    request('/accident', {
      method: 'POST',
      body: JSON.stringify({ source, target, kind: 'partial_blockage', severity: 0.72 }),
    }),
  clearAccidents: () => request('/accident', { method: 'DELETE', body: JSON.stringify({}) }),
  getStats: () => request<SimulationStats>('/stats'),
  getSimulation: () => request<SimulationState>('/simulation'),
  start: () => request<SimulationState>('/simulation/start', { method: 'POST' }),
  pause: () => request<SimulationState>('/simulation/pause', { method: 'POST' }),
  resume: () => request<SimulationState>('/simulation/resume', { method: 'POST' }),
  reset: () => request<SimulationState>('/simulation/reset', { method: 'POST' }),
  setSpeed: (speed: number) =>
    request<SimulationState>('/simulation/speed', { method: 'POST', body: JSON.stringify({ speed }) }),
  compareAlgorithms: (source: string, destination: string) =>
    request<AlgorithmComparison>(
      `/algorithms/compare?source=${encodeURIComponent(source)}&destination=${encodeURIComponent(destination)}`,
    ),
};
