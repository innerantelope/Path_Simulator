import { useEffect, useMemo, useState } from 'react';
import { WS_URL } from '../api/client';
import type { SimulationSnapshot } from '../types';

export function useSimulationSocket() {
  const [snapshot, setSnapshot] = useState<SimulationSnapshot | null>(null);
  const [connected, setConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const wsUrl = useMemo(() => WS_URL, []);

  useEffect(() => {
    let closedByEffect = false;
    let connectTimer: number | undefined;
    let reconnectTimer: number | undefined;
    let socket: WebSocket | undefined;

    const connect = () => {
      if (closedByEffect) {
        return;
      }

      socket = new WebSocket(wsUrl);

      socket.onopen = () => {
        setConnected(true);
        setError(null);
      };
      socket.onclose = () => {
        setConnected(false);
        if (!closedByEffect) {
          reconnectTimer = window.setTimeout(connect, 1200);
        }
      };
      socket.onerror = () => {
        setError(`WebSocket connection failed at ${wsUrl}`);
        socket?.close();
      };
      socket.onmessage = (event) => {
        try {
          const payload = JSON.parse(event.data) as Partial<SimulationSnapshot>;
          if (payload.type !== 'simulation_update' || !payload.simulation || !payload.stats) {
            setError('Invalid WebSocket update: missing simulation_update payload.');
            return;
          }
          setSnapshot(payload as SimulationSnapshot);
          setError(null);
        } catch (parseError) {
          setError(
            `Invalid WebSocket JSON: ${
              parseError instanceof Error ? parseError.message : 'message could not be parsed'
            }`,
          );
        }
      };
    };

    connectTimer = window.setTimeout(connect, 0);

    return () => {
      closedByEffect = true;
      if (connectTimer) {
        window.clearTimeout(connectTimer);
      }
      if (reconnectTimer) {
        window.clearTimeout(reconnectTimer);
      }
      socket?.close();
    };
  }, [wsUrl]);

  return { snapshot, connected, error };
}
