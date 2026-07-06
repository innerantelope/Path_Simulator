export type CityLayout = 'grid' | 'random' | 'circular' | 'radial';
export type VehicleType = 'car' | 'bus' | 'police' | 'fire_truck' | 'ambulance';
export type TrafficLightState = 'green' | 'yellow' | 'red';
export type Algorithm = 'A*' | 'Dijkstra' | 'BFS';

export interface Node {
  id: string;
  x: number;
  y: number;
  metadata: Record<string, unknown>;
}

export interface Edge {
  source: string;
  target: string;
  weight: number;
  metadata: Record<string, unknown>;
}

export interface GraphPayload {
  directed: boolean;
  nodes: Node[];
  edges: Edge[];
}

export interface RoadState {
  source: string;
  target: string;
  baseWeight: number;
  currentWeight: number;
  vehicleCount: number;
  averageSpeed: number;
  congestionScore: number;
  travelTime: number;
  closed: boolean;
  accident: boolean;
  accidentSeverity: number;
}

export interface TrafficLight {
  nodeId: string;
  state: TrafficLightState;
  timer: number;
  priorityHoldSeconds: number;
}

export interface Vehicle {
  id: string;
  type: VehicleType;
  emergency: boolean;
  currentNode: string;
  destinationNode: string;
  speed: number;
  currentPath: string[];
  pathIndex: number;
  edgeProgress: number;
  fuel: number;
  waitingTime: number;
  priority: number;
  position: {
    x: number;
    y: number;
  };
  routeCost: number;
  completedTrips: number;
  rerouteCount: number;
}

export interface Accident {
  id: string;
  source: string;
  target: string;
  kind: string;
  severity: number;
  active: boolean;
}

export interface SimulationStats {
  simulatedTimeSeconds: number;
  averageTravelTime: number;
  averageCongestion: number;
  vehicleThroughput: number;
  fuelConsumption: number;
  waitingTime: number;
  roadUtilization: number;
  activeVehicles: number;
  accidentCount: number;
  closedRoadCount: number;
  collisionWarnings: number;
  tick: number;
}

export interface CityResponse {
  graph: GraphPayload;
  roads: RoadState[];
  trafficLights: TrafficLight[];
}

export interface SimulationState {
  running: boolean;
  speedMultiplier: number;
  stats: SimulationStats;
  vehicleCount: number;
  roadCount: number;
}

export interface SimulationSnapshot {
  type: 'simulation_update';
  simulation: {
    running: boolean;
    speedMultiplier: number;
    tick: number;
    simulatedTimeSeconds: number;
  };
  vehicles: Vehicle[];
  trafficLights: TrafficLight[];
  roads: RoadState[];
  accidents: Accident[];
  stats: SimulationStats;
}

export interface PathResult {
  path: string[];
  visitedNodes: string[];
  travelCost: number;
  executionTimeMs: number;
  success: boolean;
  algorithm: string;
}

export type AlgorithmComparison = Record<string, PathResult>;
