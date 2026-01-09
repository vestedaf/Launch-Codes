
/*
 * MIDIChordSeq_Tie.ino — Teensy 4.0
 * USB-MIDI (device) IN: notes + Clock/Start/Stop/Continue.
 * OUT: USB-MIDI + DIN-MIDI (Serial1 @ 31250 baud).
 *
 * New: per-step Tie. Gate length fixed at 50%. With Tie active, we do NOT send NoteOff
 * at the end of the step, and we do NOT retrigger on the next step; instead the chord
 * sustains across steps until the tie chain ends.
 *
 * MIDI clock spec: 24 pulses per quarter note; 1/16 = 6 pulses.  [Wikipedia]               [1]
 * Teensy USB-MIDI clock callbacks: setHandleClock/Start/Continue/Stop.  [little-scale]    [2]
 * Teensy USB-MIDI device API & examples:                                                [PJRC] [3]
 * DIN-MIDI on Teensy (Serial1 @ 31250; OUT=2 resistors, IN=opto):                        [PJRC] [4]
 *
 * [1] https://en.wikipedia.org/wiki/MIDI_beat_clock
 * [2] https://little-scale.blogspot.com/2020/05/teensy-36-basics-receiving-midi-clock.html
 * [3] https://www.pjrc.com/teensy/td_midi.html
 * [4] https://www.pjrc.com/teensy/td_libs_MIDI.html
 */

#include <Arduino.h>
#include <MIDI.h>   // FortySevenEffects library for DIN-MIDI over UART (Serial1)

constexpr int STEPS = 64;            // grid length (expandable later)
constexpr uint8_t OUT_CHANNEL = 1;   // output channel
constexpr uint8_t TICKS_PER_STEP = 6; // 1/16 note at 24 PPQN → 6 ticks
constexpr uint8_t GATE_TICKS = 3;     // fixed gate length = 50% of step (6 * 0.5 = 3)

// --- Sequencer state ---
bool stepEnabled[STEPS];   // play chord on this step?
bool stepTie[STEPS];       // sustain into next step?

// Held notes (current input chord) and velocities
bool    held[128];
uint8_t heldVel[128];

// Last captured chord (final combination before all notes are off)
struct NoteEvent { uint8_t note; uint8_t vel; };
NoteEvent lastChord[128];
int lastChordSize = 0;

// Chord aggregation window: group near-simultaneous NoteOns
constexpr uint32_t AGGREGATION_MS = 25;
bool aggregationActive = false;
elapsedMillis aggregationTimer;

// Transport & clock-driven playback
bool playing = false;
int  stepIndex = 0;
uint8_t tickInStep = 0;

// Gate & tie state
bool gateOn = false;          // chord currently sounding?
bool tieChainActive = false;  // are we sustaining across steps?

// DIN MIDI instance (Serial1, 31250 baud)
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI_DIN);

// --- Utilities: NoteOn/NoteOff for the chord on both outputs ---
static inline void sendChordOn() {
  if (lastChordSize == 0) return;
  for (int i = 0; i < lastChordSize; ++i) {
    const uint8_t n = lastChord[i].note;
    const uint8_t v = lastChord[i].vel ? lastChord[i].vel : 96; // fallback velocity
    usbMIDI.sendNoteOn(n, v, OUT_CHANNEL);
    MIDI_DIN.sendNoteOn(n, v, OUT_CHANNEL);
  }
  gateOn = true;
  tickInStep = 0;
}

static inline void sendChordOff() {
  if (lastChordSize == 0) return;
  for (int i = 0; i < lastChordSize; ++i) {
    const uint8_t n = lastChord[i].note;
    usbMIDI.sendNoteOff(n, 0, OUT_CHANNEL);
    MIDI_DIN.sendNoteOff(n, 0, OUT_CHANNEL);
  }
  gateOn = false;
}

// --- Held-chord helpers ---
static inline void clearHeld() {
  for (int n = 0; n < 128; ++n) { held[n] = false; heldVel[n] = 0; }
}

static inline void commitChordFromHeld() {
  lastChordSize = 0;
  for (int n = 0; n < 128; ++n) {
    if (held[n]) lastChord[lastChordSize++] = { (uint8_t)n, heldVel[n] };
  }
}

// --- USB-MIDI note handlers ---
void onNoteOn(byte channel, byte note, byte velocity) {
  if (velocity > 0) {
    bool wasEmpty = true; for (int n = 0; n < 128; ++n) { if (held[n]) { wasEmpty = false; break; } }
    if (wasEmpty) { aggregationActive = true; aggregationTimer = 0; }
    held[note] = true; heldVel[note] = velocity;
  } else {
    // NoteOn with vel=0 → treat as NoteOff per MIDI convention
    held[note] = false; heldVel[note] = 0;
    bool nowEmpty = true; for (int n = 0; n < 128; ++n) { if (held[n]) { nowEmpty = false; break; } }
    if (nowEmpty) { commitChordFromHeld(); aggregationActive = false; }
  }
}

void onNoteOff(byte channel, byte note, byte velocity) {
  held[note] = false; heldVel[note] = 0;
  bool nowEmpty = true; for (int n = 0; n < 128; ++n) { if (held[n]) { nowEmpty = false; break; } }
  if (nowEmpty) { commitChordFromHeld(); aggregationActive = false; }
}

// --- USB-MIDI clock & transport ---
void onClock() {
  if (!playing) return;

  // Advance tick within current step
  tickInStep++;

  // Gate handling:
  // - If tie is active (current step tied OR we are in a tie chain), do NOT send NoteOff mid-step.
  // - Otherwise, at 50% gate (3 ticks), send NoteOff if the chord is sounding.
  const bool stepIsTied = stepTie[stepIndex];
  const bool blockGateOff = stepIsTied || tieChainActive;
  if (!blockGateOff && gateOn && tickInStep >= GATE_TICKS) {
    sendChordOff();
  }

  // Step boundary
  if (tickInStep >= TICKS_PER_STEP) {
    // End-of-step safety: if gate still on, turn notes off unless we tie forward
    if (gateOn && !stepIsTied && !tieChainActive) {
      sendChordOff();
    }

    // Compute next step & tie continuation
    const int next = (stepIndex + 1) % STEPS;
    const bool nextTied = stepTie[stepIndex];  // tie forward from current step

    // Update tie chain state
    tieChainActive = nextTied;

    // Advance to next step
    stepIndex = next;
    tickInStep = 0;

    // Start logic:
    // If NOT currently in a tie chain and the next step is enabled and we have a chord → NoteOn
    if (!tieChainActive && stepEnabled[stepIndex] && lastChordSize > 0) {
      sendChordOn();
    }
    // If we ARE in a tie chain → do NOT retrigger; keep sustaining until a step without tie ends the chain.
  }
}

void onStart() {
  playing = true;
  stepIndex = 0;
  tickInStep = 0;
  tieChainActive = false;
  if (stepEnabled[stepIndex] && lastChordSize > 0) sendChordOn();
}

void onContinue() { playing = true; }

void onStop() {
  playing = false;
  sendChordOff();
}

// --- Setup & loop ---
void setup() {
  // Basic pattern for testing: enable every 4th sixteenth, no ties by default
  for (int i = 0; i < STEPS; ++i) {
    stepEnabled[i] = (i % 4 == 0);   // 0,4,8,12,...
    stepTie[i]     = false;
  }

  clearHeld();
  lastChordSize = 0;

  MIDI_DIN.begin(MIDI_CHANNEL_OMNI); // configures Serial1 @ 31250 for DIN-MIDI

  // Register USB-MIDI handlers (notes + transport/clock)
  usbMIDI.setHandleNoteOn(onNoteOn);
  usbMIDI.setHandleNoteOff(onNoteOff);
  usbMIDI.setHandleClock(onClock);
  usbMIDI.setHandleStart(onStart);
  usbMIDI.setHandleContinue(onContinue);
  usbMIDI.setHandleStop(onStop);

  Serial.begin(115200);
  Serial.println("MIDIChordSeq_Tie ready. Commands:");
  Serial.println("  p                → toggle play/stop");
  Serial.println("  s<idx>           → toggle step enabled");
  Serial.println("  t<idx>           → toggle step tie (sustain into next step)");
  Serial.println("  dump             → print enabled/tie map");
}

void loop() {
  // Drain USB-MIDI input (required to keep buffers from filling)
  while (usbMIDI.read()) { /* callbacks fire here */ }

  // Aggregation window timeout (groups near-simultaneous note-ons only)
  if (aggregationActive && aggregationTimer >= AGGREGATION_MS) {
    aggregationActive = false;
  }

  // Minimal Serial UI
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd == "p") {
      if (playing) onStop(); else onStart();
      Serial.printf("Playing=%d, step=%d\n", playing, stepIndex);
    } else if (cmd.startsWith("s")) {
      int idx = cmd.substring(1).toInt();
      if (idx >= 0 && idx < STEPS) {
        stepEnabled[idx] = !stepEnabled[idx];
        Serial.printf("Step %d enabled=%d\n", idx, stepEnabled[idx]);
      }
    } else if (cmd.startsWith("t")) {
      int idx = cmd.substring(1).toInt();
      if (idx >= 0 && idx < STEPS) {
        stepTie[idx] = !stepTie[idx];
        Serial.printf("Step %d tie=%d\n", idx, stepTie[idx]);
      }
    } else if (cmd == "dump") {
      Serial.println("Idx : En Tie");
      for (int i = 0; i < STEPS; ++i) {
        Serial.printf("%3d :  %d   %d\n", i, stepEnabled[i], stepTie[i]);
      }
    }
  }
}
