#pragma once

#include <Arduino.h>

// Call once per loop iteration. Drives the measurement sequence and the
// idle-state motion detector.
void smTick();

// True when the device is not running a measurement sequence — safe to do
// blocking work like Firestore polling.
bool smIsIdle();
