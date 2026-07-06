import { useEffect, useMemo, useState } from 'react';
import type { ReactNode } from 'react';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { Activity, Map, RadioTower } from 'lucide-react';
import { Route, Routes } from 'react-router-dom';
import { api, formatApiError } from './api/client';
import { AnalyticsDashboard } from './components/AnalyticsDashboard';
import { ControlPanel } from './components/ControlPanel';
import { SimulationCanvas } from './components/SimulationCanvas';
import { useSimulationSocket } from './hooks/useSimulationSocket';
import type {
  AlgorithmComparison,
  CityLayout,
  RoadState,
  SimulationState,
  SimulationStats,
  VehicleType,
} from './types';

function Dashboard() {
  const queryClient = useQueryClient();
  const { snapshot, connected, error: socketError } = useSimulationSocket();
  const [selectedRoad, setSelectedRoad] = useState<RoadState | null>(null);
  const [history, setHistory] = useState<SimulationStats[]>([]);
  const [comparison, setComparison] = useState<AlgorithmComparison | null>(null);
  const [layout, setLayout] = useState<CityLayout>('grid');
  const [intersections, setIntersections] = useState(144);
  const [roadDensity, setRoadDensity] = useState(0.14);
  const [vehicles, setVehicles] = useState(80);
  const [vehicleType, setVehicleType] = useState<VehicleType>('car');

  const cityQuery = useQuery({ queryKey: ['city'], queryFn: api.getCity });
  const simulationQuery = useQuery({ queryKey: ['simulation'], queryFn: api.getSimulation, refetchInterval: 5000 });

  const stats = snapshot?.stats ?? simulationQuery.data?.stats;
  const liveSimulation: SimulationState | undefined = snapshot
    ? {
        running: snapshot.simulation.running,
        speedMultiplier: snapshot.simulation.speedMultiplier,
        stats: snapshot.stats,
        vehicleCount: snapshot.vehicles.length,
        roadCount: snapshot.roads.length,
      }
    : simulationQuery.data;

  const currentRoads = snapshot?.roads ?? cityQuery.data?.roads ?? [];
  const currentSelectedRoad = useMemo(() => {
    if (!selectedRoad) return null;
    return (
      currentRoads.find((road) => road.source === selectedRoad.source && road.target === selectedRoad.target) ??
      selectedRoad
    );
  }, [currentRoads, selectedRoad]);

  useEffect(() => {
    if (!snapshot?.stats) return;
    setHistory((current) => {
      if (current[current.length - 1]?.tick === snapshot.stats.tick) {
        return current;
      }
      return [...current.slice(-79), snapshot.stats];
    });
  }, [snapshot?.stats]);

  const invalidateLiveData = () => {
    void queryClient.invalidateQueries({ queryKey: ['city'] });
    void queryClient.invalidateQueries({ queryKey: ['simulation'] });
  };

  const generateCityMutation = useMutation({
    mutationFn: () =>
      api.generateCity({
        layout,
        intersections,
        roadDensity,
        averageRoadLength: 70,
        directed: false,
        seed: Math.floor(Date.now() % 100_000),
        vehicles,
      }),
    onSuccess: () => {
      setSelectedRoad(null);
      setComparison(null);
      setHistory([]);
      invalidateLiveData();
    },
  });

  const spawnMutation = useMutation({
    mutationFn: () => api.spawnVehicles({ count: vehicles, type: vehicleType }),
    onSuccess: invalidateLiveData,
  });

  const simpleMutation = useMutation({
    mutationFn: (action: () => Promise<unknown>) => action(),
    onSuccess: invalidateLiveData,
  });

  const compareMutation = useMutation({
    mutationFn: async () => {
      const nodes = cityQuery.data?.graph.nodes ?? [];
      if (nodes.length < 2) {
        throw new Error('Generate a city before comparing algorithms.');
      }
      const source = currentSelectedRoad?.source ?? nodes[0].id;
      const destination = currentSelectedRoad?.target ?? nodes[nodes.length - 1].id;
      return api.compareAlgorithms(source, destination);
    },
    onSuccess: setComparison,
  });

  const latestError =
    cityQuery.error ??
    simulationQuery.error ??
    generateCityMutation.error ??
    spawnMutation.error ??
    simpleMutation.error ??
    compareMutation.error;
  const error = latestError ? formatApiError(latestError) : socketError;

  return (
    <main className="grid h-screen grid-rows-[auto_1fr] overflow-hidden bg-ink text-slate-100">
      <header className="flex flex-wrap items-center justify-between gap-3 border-b border-line bg-panel px-5 py-3">
        <div className="flex items-center gap-3">
          <div className="flex h-10 w-10 items-center justify-center rounded-lg bg-accent/15 text-accent">
            <Map size={22} />
          </div>
          <div>
            <h1 className="text-xl font-semibold text-white">SmartTraffic AI</h1>
            <p className="text-sm text-slate-400">Real-time smart traffic simulation powered by A* search</p>
          </div>
        </div>
        <div className="flex items-center gap-3 text-sm text-slate-300">
          <StatusPill icon={<RadioTower size={15} />} label={connected ? 'Live stream' : 'Offline'} active={connected} />
          <StatusPill icon={<Activity size={15} />} label={liveSimulation?.running ? 'Running' : 'Paused'} active={!!liveSimulation?.running} />
        </div>
      </header>

      <div className="grid min-h-0 gap-4 p-4 xl:grid-cols-[320px_1fr]">
        <ControlPanel
          selectedRoad={currentSelectedRoad}
          simulation={liveSimulation}
          connected={connected}
          layout={layout}
          intersections={intersections}
          roadDensity={roadDensity}
          vehicles={vehicles}
          vehicleType={vehicleType}
          algorithmComparison={comparison}
          onLayoutChange={setLayout}
          onIntersectionsChange={setIntersections}
          onRoadDensityChange={setRoadDensity}
          onVehiclesChange={setVehicles}
          onVehicleTypeChange={setVehicleType}
          onGenerateCity={() => generateCityMutation.mutate()}
          onSpawnVehicles={() => spawnMutation.mutate()}
          onStart={() => simpleMutation.mutate(api.start)}
          onPause={() => simpleMutation.mutate(api.pause)}
          onReset={() => simpleMutation.mutate(api.reset)}
          onSpeed={(speed) => simpleMutation.mutate(() => api.setSpeed(speed))}
          onCloseRoad={() =>
            currentSelectedRoad && simpleMutation.mutate(() => api.closeRoad(currentSelectedRoad.source, currentSelectedRoad.target))
          }
          onOpenRoad={() =>
            currentSelectedRoad && simpleMutation.mutate(() => api.openRoad(currentSelectedRoad.source, currentSelectedRoad.target))
          }
          onCreateAccident={() =>
            simpleMutation.mutate(() => api.createAccident(currentSelectedRoad?.source, currentSelectedRoad?.target))
          }
          onClearAccidents={() => simpleMutation.mutate(api.clearAccidents)}
          onCompareAlgorithms={() => compareMutation.mutate()}
        />

        <section className="grid min-h-0 grid-rows-[1fr_auto] gap-4">
          {error && (
            <div className="rounded-lg border border-danger/50 bg-danger/10 px-4 py-2 text-sm text-danger">
              {error}
            </div>
          )}
          <SimulationCanvas
            city={cityQuery.data}
            snapshot={snapshot}
            selectedRoad={currentSelectedRoad}
            onSelectRoad={setSelectedRoad}
          />
          <AnalyticsDashboard stats={stats} history={history} />
        </section>
      </div>
    </main>
  );
}

function StatusPill({ icon, label, active }: { icon: ReactNode; label: string; active: boolean }) {
  return (
    <div className={`inline-flex items-center gap-2 rounded-full border px-3 py-1 ${active ? 'border-success/50 bg-success/10 text-success' : 'border-line bg-ink text-slate-400'}`}>
      {icon}
      {label}
    </div>
  );
}

export default function App() {
  return (
    <Routes>
      <Route path="/" element={<Dashboard />} />
    </Routes>
  );
}
