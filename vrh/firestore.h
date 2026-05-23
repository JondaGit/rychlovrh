#pragma once

#include <Arduino.h>

struct DeviceSettings {
  int gyroSensitivity;  // % threshold of MAX_DELTA_G
  int volume;           // 0..100 for the detection buzzer
  int minDelay;         // ms — minimum armed-wait before GO beep
  int maxDelay;         // ms — maximum armed-wait before GO beep
};

struct PendingCommand {
  String id;    // empty = no pending command
  String type;  // e.g. "measure"
};

extern DeviceSettings settings;
extern PendingCommand pendingCommand;

// Last commandId we know has been processed (read from the doc on each fetch).
// Used to skip stale commands across reboots.
extern String lastResultCommandId;

// Force a one-shot fetch. Returns true on success. Updates settings + pendingCommand in place.
bool fetchSettings();

// Call from loop(). Fetches if the poll interval has elapsed since the last attempt.
void maybeFetchSettings();

// PATCH the device doc with the result of a measurement. durationMs < 0 means timeout.
bool publishResult(const String& commandId, int durationMs);
