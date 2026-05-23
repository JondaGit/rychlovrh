import { useEffect, useState } from "react";
import type { DeviceData, Settings as SettingsType } from "../types";

type Props = {
  device: {
    data: DeviceData | null;
    updateSettings: (patch: Partial<SettingsType>) => Promise<void>;
  };
  onBack: () => void;
};

const FIELDS = [
  { key: "gyroSensitivity", label: "Citlivost gyroskopu", kind: "slider", min: 1, max: 100, suffix: "%" },
  { key: "volume",          label: "Hlasitost",           kind: "slider", min: 0, max: 100, suffix: "%" },
  { key: "minDelay",        label: "Min. prodleva",       kind: "number", suffix: "MS", step: 100 },
  { key: "maxDelay",        label: "Max. prodleva",       kind: "number", suffix: "MS", step: 100 },
] as const;

export function Settings({ device, onBack }: Props) {
  const [draft, setDraft] = useState<SettingsType | null>(null);
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    if (device.data && !draft) {
      setDraft({
        gyroSensitivity: device.data.gyroSensitivity,
        volume: device.data.volume,
        minDelay: device.data.minDelay,
        maxDelay: device.data.maxDelay,
      });
    }
  }, [device.data, draft]);

  const onSave = async () => {
    if (!draft) return;
    setSaving(true);
    try {
      await device.updateSettings(draft);
      onBack();
    } finally {
      setSaving(false);
    }
  };

  return (
    <div className="page page--settings">
      <header className="header">
        <button
          className="icon-btn"
          onClick={onBack}
          aria-label="Zpět"
        >
          <BackIcon />
        </button>
        <div className="brand">
          <span className="brand__text">NASTAVENÍ</span>
        </div>
        <span className="header__spacer" />
      </header>

      {!draft ? (
        <div className="loading">NAČÍTÁM…</div>
      ) : (
        <>
          <main className="settings">
            {FIELDS.map((f, i) => {
              const value = draft[f.key];
              const set = (v: number) => setDraft({ ...draft, [f.key]: v });
              const idx = String(i + 1).padStart(2, "0");

              if (f.kind === "slider") {
                return (
                  <div className="field" key={f.key}>
                    <span className="field__label">
                      <span className="field__index">{idx}</span>
                      {f.label}
                    </span>
                    <div className="field__row">
                      <input
                        type="range"
                        className="slider"
                        min={f.min}
                        max={f.max}
                        value={value}
                        onChange={(e) => set(Number(e.target.value))}
                      />
                      <span className="field__value">
                        {value}
                        <span className="field__suffix">{f.suffix}</span>
                      </span>
                    </div>
                  </div>
                );
              }

              return (
                <div className="field" key={f.key}>
                  <span className="field__label">
                    <span className="field__index">{idx}</span>
                    {f.label}
                  </span>
                  <div className="field__row">
                    <input
                      type="number"
                      inputMode="numeric"
                      className="number-input"
                      value={value}
                      step={f.step}
                      min={0}
                      onChange={(e) => set(Number(e.target.value))}
                    />
                    <span className="field__suffix field__suffix--inline">
                      {f.suffix}
                    </span>
                  </div>
                </div>
              );
            })}
          </main>

          <footer className="cta">
            <button
              className="throw-btn"
              onClick={onSave}
              disabled={saving}
            >
              {saving ? "UKLÁDÁM" : "ULOŽIT"}
            </button>
          </footer>
        </>
      )}
    </div>
  );
}

function BackIcon() {
  return (
    <svg
      width="20"
      height="20"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.5"
      strokeLinecap="square"
      strokeLinejoin="miter"
    >
      <path d="M19 12H5M12 19l-7-7 7-7" />
    </svg>
  );
}
