import { useEffect, useRef, useState } from "react";
import { motion } from "motion/react";
import type { DeviceData } from "../types";

type Props = {
  device: {
    data: DeviceData | null;
    triggerMeasurement: () => Promise<string>;
  };
  onSettings: () => void;
};

function formatMs(value: number): string {
  if (value < 0) return "—";
  // Czech thousands separator = thin space
  return value.toLocaleString("cs-CZ").replace(/ /g, " ");
}

export function Home({ device, onSettings }: Props) {
  const [pendingCmd, setPendingCmd] = useState<string | null>(null);
  const timeoutRef = useRef<number | null>(null);

  const lastMs = device.data?.lastResultMs;
  const lastCmdId = device.data?.lastResultCommandId;
  const online = device.data !== null;

  // Clear pending when our command's result lands.
  useEffect(() => {
    if (pendingCmd && lastCmdId === pendingCmd) {
      setPendingCmd(null);
      if (timeoutRef.current !== null) {
        clearTimeout(timeoutRef.current);
        timeoutRef.current = null;
      }
    }
  }, [pendingCmd, lastCmdId]);

  // Hard fallback in case the device never responds (offline, error).
  useEffect(() => {
    if (!pendingCmd) return;
    timeoutRef.current = window.setTimeout(() => {
      setPendingCmd(null);
      timeoutRef.current = null;
    }, 60_000);
    return () => {
      if (timeoutRef.current !== null) clearTimeout(timeoutRef.current);
    };
  }, [pendingCmd]);

  const onThrow = async () => {
    if (pendingCmd) return;
    try {
      const id = await device.triggerMeasurement();
      setPendingCmd(id);
    } catch (e) {
      console.error("trigger failed", e);
    }
  };

  const isTimeout = !pendingCmd && lastMs !== undefined && lastMs < 0;
  const hasResult = !pendingCmd && lastMs !== undefined && lastMs >= 0;

  const labelText = pendingCmd
    ? "MĚŘENÍ"
    : isTimeout
      ? "VYPRŠEL ČAS"
      : "POSLEDNÍ ČAS";

  const numberText = pendingCmd
    ? "— — —"
    : !hasResult
      ? "—"
      : formatMs(lastMs!);

  const numberKey = pendingCmd ?? `${lastMs}-${lastCmdId}`;

  return (
    <div className="page page--home">
      <header className="header">
        <div className="brand">
          <span className="brand__dot" data-online={online} />
          <span className="brand__text">SVIŠTÍ&nbsp;VRH</span>
        </div>
        <button
          className="icon-btn"
          onClick={onSettings}
          aria-label="Nastavení"
        >
          <GearIcon />
        </button>
      </header>

      <main className="result">
        <div className="result__label">{labelText}</div>
        <div className="result__numberWrap">
          <motion.div
            key={numberKey}
            className="result__number"
            initial={{ opacity: 0, y: 14, scale: 0.98 }}
            animate={{ opacity: 1, y: 0, scale: 1 }}
            transition={{ duration: 0.32, ease: [0.4, 0, 0.2, 1] }}
          >
            {numberText}
          </motion.div>
        </div>
        <div className={`result__unit ${pendingCmd ? "result__unit--dim" : ""}`}>
          MS
        </div>
        <div
          className={`ruler ${pendingCmd ? "ruler--active" : ""}`}
          aria-hidden
        />
      </main>

      <footer className="cta">
        <button
          className={`throw-btn ${pendingCmd ? "throw-btn--pending" : ""}`}
          onClick={onThrow}
          disabled={!!pendingCmd}
        >
          {pendingCmd ? "ČEKÁM" : "VRHNI"}
        </button>
      </footer>
    </div>
  );
}

function GearIcon() {
  return (
    <svg
      width="20"
      height="20"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.5"
      strokeLinecap="square"
    >
      <circle cx="12" cy="12" r="3" />
      <path d="M12 1.5v3M12 19.5v3M4.2 4.2l2.1 2.1M17.7 17.7l2.1 2.1M1.5 12h3M19.5 12h3M4.2 19.8l2.1-2.1M17.7 6.3l2.1-2.1" />
    </svg>
  );
}
