import type {
  AlgorithmComparison,
  CityLayout,
  CityResponse,
  SimulationState,
  SimulationStats,
  Vehicle,
  VehicleType,
} from '../types';

const DEFAULT_API_BASE_URL = 'http://localhost:18080';
const DEFAULT_WS_BASE_URL = 'ws://localhost:18080';
const DEFAULT_TIMEOUT_MS = 10_000;

export type ApiErrorKind = 'http' | 'timeout' | 'connection_refused' | 'network' | 'invalid_json';

export class ApiError extends Error {
  readonly kind: ApiErrorKind;
  readonly status?: number;
  readonly method: string;
  readonly url: string;
  readonly details?: string;

  constructor({
    kind,
    message,
    method,
    url,
    status,
    details,
  }: {
    kind: ApiErrorKind;
    message: string;
    method: string;
    url: string;
    status?: number;
    details?: string;
  }) {
    super(message);
    this.name = 'ApiError';
    this.kind = kind;
    this.method = method;
    this.url = url;
    this.status = status;
    this.details = details;
  }
}

function configuredEnv(name: 'VITE_API_BASE_URL' | 'VITE_WS_BASE_URL') {
  const value = import.meta.env[name] as string | undefined;
  return value?.trim() || undefined;
}

function stripTrailingSlashes(value: string) {
  return value.replace(/\/+$/, '');
}

function normalizeWsBaseUrl(value: string) {
  return stripTrailingSlashes(value).replace(/\/ws$/i, '');
}

export const API_BASE_URL = stripTrailingSlashes(configuredEnv('VITE_API_BASE_URL') ?? DEFAULT_API_BASE_URL);
export const WS_BASE_URL = normalizeWsBaseUrl(configuredEnv('VITE_WS_BASE_URL') ?? DEFAULT_WS_BASE_URL);
export const WS_URL = `${WS_BASE_URL}/ws`;

function isLocalhost(url: string) {
  try {
    const hostname = new URL(url).hostname;
    return hostname === 'localhost' || hostname === '127.0.0.1' || hostname === '::1';
  } catch {
    return false;
  }
}

function statusLabel(status: number) {
  if (status === 404) return '404 Not Found';
  if (status >= 500) return `${status} Server Error`;
  return `${status} Request Failed`;
}

export function formatApiError(error: unknown) {
  if (!(error instanceof ApiError)) {
    return error instanceof Error ? error.message : 'Unexpected error';
  }

  const path = (() => {
    try {
      const parsed = new URL(error.url);
      return `${parsed.pathname}${parsed.search}`;
    } catch {
      return error.url;
    }
  })();

  switch (error.kind) {
    case 'http':
      return `${statusLabel(error.status ?? 0)} on ${error.method} ${path}: ${error.message}`;
    case 'timeout':
      return `Network timeout on ${error.method} ${path}: the API did not respond within ${DEFAULT_TIMEOUT_MS / 1000}s.`;
    case 'connection_refused':
      return `Connection refused on ${error.method} ${path}: the API is not reachable at ${API_BASE_URL}.`;
    case 'invalid_json':
      return `Invalid JSON from ${error.method} ${path}: ${error.message}`;
    case 'network':
      return `Network error on ${error.method} ${path}: ${error.message}`;
  }
}

async function request<T>(path: string, options: RequestInit = {}): Promise<T> {
  const method = (options.method ?? 'GET').toUpperCase();
  const url = `${API_BASE_URL}${path}`;
  const headers = new Headers(options.headers);
  headers.set('Accept', 'application/json');
  if (options.body !== undefined && !headers.has('Content-Type')) {
    headers.set('Content-Type', 'application/json');
  }

  const controller = new AbortController();
  const timeoutId = window.setTimeout(() => controller.abort(), DEFAULT_TIMEOUT_MS);
  let response: Response;

  try {
    response = await fetch(url, {
      ...options,
      headers,
      signal: controller.signal,
    });
  } catch (error) {
    if (error instanceof DOMException && error.name === 'AbortError') {
      throw new ApiError({
        kind: 'timeout',
        message: 'request timed out',
        method,
        url,
      });
    }

    if (error instanceof TypeError && isLocalhost(url)) {
      throw new ApiError({
        kind: 'connection_refused',
        message: error.message,
        method,
        url,
      });
    }

    throw new ApiError({
      kind: 'network',
      message: error instanceof Error ? error.message : 'request failed before receiving a response',
      method,
      url,
    });
  } finally {
    window.clearTimeout(timeoutId);
  }

  const text = await response.text();
  let payload: unknown = {};
  if (text) {
    try {
      payload = JSON.parse(text);
    } catch (error) {
      if (response.ok) {
        throw new ApiError({
          kind: 'invalid_json',
          message: error instanceof Error ? error.message : 'response body is not valid JSON',
          method,
          url,
          details: text,
        });
      }
      payload = { error: text };
    }
  }

  if (!response.ok) {
    const message =
      payload && typeof payload === 'object' && 'error' in payload && typeof payload.error === 'string'
        ? payload.error
        : response.statusText || 'request failed';
    throw new ApiError({
      kind: 'http',
      message,
      method,
      url,
      status: response.status,
      details: text,
    });
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
