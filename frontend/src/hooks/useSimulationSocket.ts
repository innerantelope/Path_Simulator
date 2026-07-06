import { useEffect, useMemo, useState } from 'react';
import { API_URL } from '../api/client';
import type { SimulationSnapshot } from '../types';

export function useSimulationSocket() {
  const [snapshot, setSnapshot] = useState<SimulationSnapshot | null>(null);
  const [connected, setConnected] = useState(false);

  const wsUrl = useMemo(() => {
    const configured = import.meta.env.VITE_WS_URL as string | undefined;
    if (configured) {
      return configured;
    }
    return `${API_URL.replace(/^http/, 'ws')}/ws`;
  }, []);

  useEffect(() => {
    let closedByEffect = false;
    let reconnectTimer: number | undefined;
    let socket: WebSocket | undefined;

    const connect = () => {
      socket = new WebSocket(wsUrl);

      socket.onopen = () => setConnected(true);
      socket.onclose = () => {
        setConnected(false);
        if (!closedByEffect) {
          reconnectTimer = window.setTimeout(connect, 1200);
        }
      };
      socket.onerror = () => {
        socket?.close();
      };
      socket.onmessage = (event) => {
        const payload = JSON.parse(event.data) as SimulationSnapshot;
        if (payload.type === 'simulation_update') {
          setSnapshot(payload);
        }
      };
    };

    connect();

    return () => {
      closedByEffect = true;
      if (reconnectTimer) {
        window.clearTimeout(reconnectTimer);
      }
      socket?.close();
    };
  }, [wsUrl]);

  return { snapshot, connected };
}
