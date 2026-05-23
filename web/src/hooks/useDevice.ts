import { useEffect, useState } from "react";
import { onSnapshot, updateDoc } from "firebase/firestore";
import { deviceRef } from "../firebase";
import type { DeviceData, Settings } from "../types";

export function useDevice() {
  const [data, setData] = useState<DeviceData | null>(null);

  useEffect(() => {
    return onSnapshot(deviceRef, (snap) => {
      const d = snap.data();
      if (d) setData(d as DeviceData);
    });
  }, []);

  return {
    data,
    updateSettings: (patch: Partial<Settings>) => updateDoc(deviceRef, patch),
    triggerMeasurement: async () => {
      const id = `web-${Date.now()}`;
      await updateDoc(deviceRef, {
        command: { id, type: "measure" },
      });
      return id;
    },
  };
}
