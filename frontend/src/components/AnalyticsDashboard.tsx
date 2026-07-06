import {
  BarElement,
  CategoryScale,
  Chart as ChartJS,
  Legend,
  LinearScale,
  LineElement,
  PointElement,
  Tooltip,
} from 'chart.js';
import { Bar, Line } from 'react-chartjs-2';
import type { SimulationStats } from '../types';

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, BarElement, Tooltip, Legend);

interface AnalyticsDashboardProps {
  stats?: SimulationStats;
  history: SimulationStats[];
}

const compact = (value: number) =>
  new Intl.NumberFormat('en', {
    maximumFractionDigits: value >= 10 ? 0 : 2,
  }).format(value);

export function AnalyticsDashboard({ stats, history }: AnalyticsDashboardProps) {
  const labels = history.map((point) => String(point.tick));

  const congestionData = {
    labels,
    datasets: [
      {
        label: 'Congestion',
        data: history.map((point) => point.averageCongestion * 100),
        borderColor: '#23c7d9',
        backgroundColor: 'rgba(35, 199, 217, 0.2)',
        tension: 0.35,
        pointRadius: 0,
      },
      {
        label: 'Road utilization',
        data: history.map((point) => point.roadUtilization * 100),
        borderColor: '#2ddf8f',
        backgroundColor: 'rgba(45, 223, 143, 0.18)',
        tension: 0.35,
        pointRadius: 0,
      },
    ],
  };

  const operationsData = {
    labels: ['Travel time', 'Throughput', 'Fuel', 'Waiting'],
    datasets: [
      {
        label: 'Current',
        data: [
          stats?.averageTravelTime ?? 0,
          stats?.vehicleThroughput ?? 0,
          stats?.fuelConsumption ?? 0,
          stats?.waitingTime ?? 0,
        ],
        backgroundColor: ['#23c7d9', '#2ddf8f', '#f8c14a', '#ff5c7a'],
        borderRadius: 6,
      },
    ],
  };

  const options = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        labels: {
          color: '#cbd5e1',
          boxWidth: 10,
        },
      },
    },
    scales: {
      x: {
        ticks: { color: '#94a3b8', maxTicksLimit: 5 },
        grid: { color: 'rgba(148, 163, 184, 0.08)' },
      },
      y: {
        ticks: { color: '#94a3b8' },
        grid: { color: 'rgba(148, 163, 184, 0.08)' },
      },
    },
  } as const;

  return (
    <section className="grid gap-4 lg:grid-cols-[1fr_1.1fr]">
      <div className="grid grid-cols-2 gap-3 sm:grid-cols-4 lg:grid-cols-2 xl:grid-cols-4">
        <Metric label="Vehicles" value={stats?.activeVehicles ?? 0} />
        <Metric label="Throughput" value={stats?.vehicleThroughput ?? 0} />
        <Metric label="Congestion" value={`${compact((stats?.averageCongestion ?? 0) * 100)}%`} />
        <Metric label="Warnings" value={stats?.collisionWarnings ?? 0} />
      </div>

      <div className="grid gap-4 md:grid-cols-2">
        <div className="h-44 rounded-lg border border-line bg-panel p-3">
          <h3 className="mb-2 text-sm font-semibold text-white">Network Load</h3>
          <Line data={congestionData} options={options} />
        </div>
        <div className="h-44 rounded-lg border border-line bg-panel p-3">
          <h3 className="mb-2 text-sm font-semibold text-white">Operations</h3>
          <Bar data={operationsData} options={options} />
        </div>
      </div>
    </section>
  );
}

function Metric({ label, value }: { label: string; value: number | string }) {
  return (
    <div className="rounded-lg border border-line bg-panel px-4 py-3">
      <div className="text-xs uppercase tracking-[0.18em] text-slate-500">{label}</div>
      <div className="mt-2 text-2xl font-semibold text-white">{typeof value === 'number' ? compact(value) : value}</div>
    </div>
  );
}
