import {
  AlertTriangle,
  Ban,
  Gauge,
  GitCompare,
  Pause,
  Play,
  Plus,
  RotateCcw,
  Shuffle,
  Unlock,
} from 'lucide-react';
import type { AlgorithmComparison, CityLayout, RoadState, SimulationState, VehicleType } from '../types';

interface ControlPanelProps {
  selectedRoad: RoadState | null;
  simulation?: SimulationState;
  connected: boolean;
  layout: CityLayout;
  intersections: number;
  roadDensity: number;
  vehicles: number;
  vehicleType: VehicleType;
  algorithmComparison: AlgorithmComparison | null;
  onLayoutChange: (layout: CityLayout) => void;
  onIntersectionsChange: (value: number) => void;
  onRoadDensityChange: (value: number) => void;
  onVehiclesChange: (value: number) => void;
  onVehicleTypeChange: (value: VehicleType) => void;
  onGenerateCity: () => void;
  onSpawnVehicles: () => void;
  onStart: () => void;
  onPause: () => void;
  onReset: () => void;
  onSpeed: (speed: number) => void;
  onCloseRoad: () => void;
  onOpenRoad: () => void;
  onCreateAccident: () => void;
  onClearAccidents: () => void;
  onCompareAlgorithms: () => void;
}

const buttonBase =
  'inline-flex h-9 items-center justify-center gap-2 rounded-md border border-line px-3 text-sm font-medium text-slate-100 transition hover:border-accent hover:bg-accent/10 disabled:cursor-not-allowed disabled:opacity-45';

export function ControlPanel({
  selectedRoad,
  simulation,
  connected,
  layout,
  intersections,
  roadDensity,
  vehicles,
  vehicleType,
  algorithmComparison,
  onLayoutChange,
  onIntersectionsChange,
  onRoadDensityChange,
  onVehiclesChange,
  onVehicleTypeChange,
  onGenerateCity,
  onSpawnVehicles,
  onStart,
  onPause,
  onReset,
  onSpeed,
  onCloseRoad,
  onOpenRoad,
  onCreateAccident,
  onClearAccidents,
  onCompareAlgorithms,
}: ControlPanelProps) {
  return (
    <aside className="flex h-full flex-col gap-4 overflow-y-auto rounded-lg border border-line bg-panel p-4 scrollbar-thin">
      <div className="flex items-center justify-between">
        <div>
          <h2 className="text-sm font-semibold uppercase tracking-[0.18em] text-accent">Controls</h2>
          <p className="text-xs text-slate-400">{connected ? 'WebSocket live' : 'Reconnecting stream'}</p>
        </div>
        <div className={`h-2.5 w-2.5 rounded-full ${connected ? 'bg-success' : 'bg-danger'}`} />
      </div>

      <div className="grid grid-cols-2 gap-2">
        <button className={buttonBase} onClick={onStart}>
          <Play size={16} /> Start
        </button>
        <button className={buttonBase} onClick={onPause}>
          <Pause size={16} /> Pause
        </button>
        <button className={buttonBase} onClick={onReset}>
          <RotateCcw size={16} /> Reset
        </button>
        <button className={buttonBase} onClick={onCreateAccident}>
          <AlertTriangle size={16} /> Accident
        </button>
      </div>

      <div>
        <label className="mb-2 flex items-center gap-2 text-xs font-semibold uppercase tracking-[0.18em] text-slate-400">
          <Gauge size={14} /> Simulation Speed
        </label>
        <div className="grid grid-cols-4 gap-2">
          {[1, 2, 5, 10].map((speed) => (
            <button
              key={speed}
              className={`${buttonBase} ${simulation?.speedMultiplier === speed ? 'border-accent bg-accent/15 text-white' : ''}`}
              onClick={() => onSpeed(speed)}
            >
              {speed}x
            </button>
          ))}
        </div>
      </div>

      <div className="space-y-3 rounded-md border border-line bg-ink/50 p-3">
        <div className="flex items-center justify-between">
          <h3 className="text-sm font-semibold text-white">Generate City</h3>
          <button className={buttonBase} onClick={onGenerateCity}>
            <Shuffle size={16} /> Build
          </button>
        </div>

        <label className="block text-xs text-slate-400">
          Layout
          <select
            className="mt-1 h-9 w-full rounded-md border border-line bg-panel px-2 text-sm text-white"
            value={layout}
            onChange={(event) => onLayoutChange(event.target.value as CityLayout)}
          >
            <option value="grid">Grid</option>
            <option value="random">Random</option>
            <option value="circular">Circular</option>
            <option value="radial">Radial</option>
          </select>
        </label>

        <label className="block text-xs text-slate-400">
          Intersections
          <input
            className="mt-1 h-9 w-full rounded-md border border-line bg-panel px-2 text-sm text-white"
            min={16}
            max={1000}
            type="number"
            value={intersections}
            onChange={(event) => onIntersectionsChange(Number(event.target.value))}
          />
        </label>

        <label className="block text-xs text-slate-400">
          Road density {(roadDensity * 100).toFixed(0)}%
          <input
            className="mt-1 w-full accent-accent"
            min={0.02}
            max={0.45}
            step={0.01}
            type="range"
            value={roadDensity}
            onChange={(event) => onRoadDensityChange(Number(event.target.value))}
          />
        </label>
      </div>

      <div className="space-y-3 rounded-md border border-line bg-ink/50 p-3">
        <div className="flex items-center justify-between">
          <h3 className="text-sm font-semibold text-white">Vehicles</h3>
          <button className={buttonBase} onClick={onSpawnVehicles}>
            <Plus size={16} /> Spawn
          </button>
        </div>
        <div className="grid grid-cols-2 gap-2">
          <label className="block text-xs text-slate-400">
            Count
            <input
              className="mt-1 h-9 w-full rounded-md border border-line bg-panel px-2 text-sm text-white"
              min={1}
              max={500}
              type="number"
              value={vehicles}
              onChange={(event) => onVehiclesChange(Number(event.target.value))}
            />
          </label>
          <label className="block text-xs text-slate-400">
            Type
            <select
              className="mt-1 h-9 w-full rounded-md border border-line bg-panel px-2 text-sm text-white"
              value={vehicleType}
              onChange={(event) => onVehicleTypeChange(event.target.value as VehicleType)}
            >
              <option value="car">Car</option>
              <option value="bus">Bus</option>
              <option value="police">Police</option>
              <option value="fire_truck">Fire truck</option>
              <option value="ambulance">Ambulance</option>
            </select>
          </label>
        </div>
      </div>

      <div className="space-y-3 rounded-md border border-line bg-ink/50 p-3">
        <h3 className="text-sm font-semibold text-white">Selected Road</h3>
        <div className="min-h-10 text-sm text-slate-300">
          {selectedRoad ? `${selectedRoad.source} to ${selectedRoad.target}` : 'Pick a road on the canvas'}
        </div>
        <div className="grid grid-cols-2 gap-2">
          <button className={buttonBase} disabled={!selectedRoad} onClick={onCloseRoad}>
            <Ban size={16} /> Close
          </button>
          <button className={buttonBase} disabled={!selectedRoad} onClick={onOpenRoad}>
            <Unlock size={16} /> Open
          </button>
        </div>
        <button className={`${buttonBase} w-full`} onClick={onClearAccidents}>
          Clear Accidents
        </button>
      </div>

      <div className="space-y-3 rounded-md border border-line bg-ink/50 p-3">
        <div className="flex items-center justify-between">
          <h3 className="text-sm font-semibold text-white">Algorithms</h3>
          <button className={buttonBase} onClick={onCompareAlgorithms}>
            <GitCompare size={16} /> Compare
          </button>
        </div>
        {algorithmComparison ? (
          <div className="space-y-2">
            {Object.entries(algorithmComparison).map(([name, result]) => (
              <div key={name} className="rounded-md border border-line bg-panel px-3 py-2 text-xs">
                <div className="flex items-center justify-between text-white">
                  <span>{name}</span>
                  <span>{result.success ? `${result.travelCost.toFixed(1)} cost` : 'no path'}</span>
                </div>
                <div className="mt-1 text-slate-400">
                  {result.visitedNodes.length} visited · {result.executionTimeMs.toFixed(3)} ms
                </div>
              </div>
            ))}
          </div>
        ) : (
          <p className="text-xs text-slate-400">Compares first and last city nodes.</p>
        )}
      </div>
    </aside>
  );
}
