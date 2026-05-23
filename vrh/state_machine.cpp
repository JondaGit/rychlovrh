#include "state_machine.h"

#include <Arduino.h>
#include <math.h>

#include "firestore.h"

// Implemented in vrh.ino — pull in by forward declaration to avoid splitting
// more modules than necessary right now.
extern void buzzerOn();
extern void buzzerOff();
extern void playBeep(int durationMs);
extern bool readAccelRaw(int16_t &ax, int16_t &ay, int16_t &az);

// Tuning constants. Kept local — these aren't user-tunable settings.
static const float         MAX_DELTA_G            = 2.0f;
static const unsigned long COUNTDOWN_DURATION_MS  = 3000;
static const int           SHORT_BEEP_MS          = 100;
static const unsigned long THROW_BEEP_MS          = 300;
static const unsigned long MEASUREMENT_TIMEOUT_MS = 30000;

enum class State : uint8_t {
  Idle,
  Countdown,
  ArmedWait,
  Measuring,
  Reporting
};

static State         state                  = State::Idle;
static unsigned long stateEnteredAt         = 0;
static unsigned long armedWaitDurationMs    = 0;
static unsigned long goBeepStartMs          = 0;
static bool          goBeepActive           = false;
static unsigned long measurementStartMs     = 0;
static int           lastResultMs           = -1;
static String        activeCommandId;
static String        lastHandledCommandId;

static void enterState(State next) {
  state = next;
  stateEnteredAt = millis();
}

static int gyroPercent() {
  int16_t ax, ay, az;
  if (!readAccelRaw(ax, ay, az)) return -1;

  float gx = ax / 16384.0f;
  float gy = ay / 16384.0f;
  float gz = az / 16384.0f;
  float gTotal = sqrtf(gx * gx + gy * gy + gz * gz);

  float delta = gTotal - 1.0f;
  if (delta < 0.0f) delta = 0.0f;
  if (delta > MAX_DELTA_G) delta = MAX_DELTA_G;

  return (int)roundf((delta / MAX_DELTA_G) * 100.0f);
}

bool smIsIdle() {
  return state == State::Idle;
}

static void tickIdle() {
  // Honour an unhandled measurement command if one is pending.
  // Dedup against the in-RAM lastHandledCommandId (within this session) AND
  // against lastResultCommandId from Firestore (survives reboots — a command
  // we already published a result for must not re-trigger on the next boot).
  if (pendingCommand.id.length() > 0
      && pendingCommand.id != lastHandledCommandId
      && pendingCommand.id != lastResultCommandId
      && pendingCommand.type == "measure") {
    activeCommandId = pendingCommand.id;
    Serial.print("Measurement triggered: cmd=");
    Serial.println(activeCommandId);
    buzzerOff();
    enterState(State::Countdown);
    return;
  }

  // Stay silent while idle — the original motion-detector behaviour was a dev
  // test, not useful now that the device has a defined measurement role.
  delay(50);
}

static void tickCountdown() {
  if (millis() - stateEnteredAt < COUNTDOWN_DURATION_MS) return;

  // "Get ready" — two short beeps; blocking is fine, nothing being measured yet.
  playBeep(SHORT_BEEP_MS);
  delay(80);
  playBeep(SHORT_BEEP_MS);

  int lo = settings.minDelay;
  int hi = settings.maxDelay;
  if (hi < lo) hi = lo;  // guard against bad settings
  armedWaitDurationMs = (unsigned long)random(lo, hi + 1);
  Serial.print("Armed; THROW in ");
  Serial.print(armedWaitDurationMs);
  Serial.println(" ms");

  enterState(State::ArmedWait);
}

static void tickArmedWait() {
  if (millis() - stateEnteredAt < armedWaitDurationMs) return;

  // Start the long THROW beep (non-blocking) and the timer simultaneously.
  // Reference time = beep onset (when the thrower perceives the cue).
  buzzerOn();
  goBeepStartMs = millis();
  goBeepActive  = true;
  measurementStartMs = goBeepStartMs;
  Serial.println("THROW");

  enterState(State::Measuring);
}

static void tickMeasuring() {
  unsigned long now = millis();

  if (goBeepActive && now - goBeepStartMs >= THROW_BEEP_MS) {
    buzzerOff();
    goBeepActive = false;
  }

  int pct = gyroPercent();
  if (pct >= settings.gyroSensitivity) {
    lastResultMs = (int)(now - measurementStartMs);
    Serial.print("Throw detected: ");
    Serial.print(lastResultMs);
    Serial.println(" ms");
    if (goBeepActive) {
      buzzerOff();
      goBeepActive = false;
    }
    playBeep(SHORT_BEEP_MS);  // confirmation
    enterState(State::Reporting);
    return;
  }

  if (now - measurementStartMs >= MEASUREMENT_TIMEOUT_MS) {
    Serial.println("Measurement timeout");
    lastResultMs = -1;
    if (goBeepActive) {
      buzzerOff();
      goBeepActive = false;
    }
    enterState(State::Reporting);
  }
}

static void tickReporting() {
  publishResult(activeCommandId, lastResultMs);
  lastHandledCommandId = activeCommandId;
  activeCommandId = "";
  enterState(State::Idle);
}

void smTick() {
  switch (state) {
    case State::Idle:      tickIdle();      break;
    case State::Countdown: tickCountdown(); break;
    case State::ArmedWait: tickArmedWait(); break;
    case State::Measuring: tickMeasuring(); break;
    case State::Reporting: tickReporting(); break;
  }
}
