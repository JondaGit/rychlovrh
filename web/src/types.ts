export type Command = {
  id: string;
  type: "measure";
};

export type Settings = {
  gyroSensitivity: number;
  volume: number;
  minDelay: number;
  maxDelay: number;
};

export type DeviceData = Settings & {
  command?: Command;
  lastResultMs?: number;
  lastResultCommandId?: string;
};
