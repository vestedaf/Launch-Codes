
# Tie Behavior & Timing (MIDI clock @ 24 PPQN)

## Timing basics
- MIDI beat clock sends **24 pulses per quarter note**.  
- A **sixteenth note** = **6 pulses**.  
- Our sequencer advances one step every **6 ticks** (fixed 1/16). [3](https://www.pjrc.com/teensy/td_uart.html)

## Gate vs. Tie
- **Gate length** (fixed at **50%**) = how long the chord is held **within** a step.
  - With 6 ticks per step, gate = **3 ticks** (half the step).
- **Tie** = **do not** send Note Off at the end of the step and **do not** retrigger on the next step; continue holding the chord until the tie chain ends.

## ASCII timeline (one step = 6 ticks)

### Non‑tied step (Gate 50%)
tick: 0 1 2 3 4 5
     |-----|-----|
Note: ON           OFF

### Step tied forward (sustain into next step)
Step N          Step N+1
tick: 0 1 2 3 4 5 | 0 1 2 3 4 5
     |-----|-----| |-----|-----|
Note: ON  (hold across)        OFF

- At **Step N**: Note **ON** at tick 0; **NO** Note Off at tick 3 or 5 (tie overrides gate).  
- At **Step N+1**: **NO retrigger**; continue holding.  
- When a non‑tied step is reached, send **OFF** at its end.

## Transport & callbacks
Teensy USB‑MIDI provides callbacks you register for:
- `Clock` (0xF8), `Start` (0xFA), `Continue` (0xFB), `Stop` (0xFC).  
- You must call `usbMIDI.read()` regularly for handlers to fire and buffers to drain. [6](https://docs.github.com/en/get-started/git-basics/git-workflows)[5](https://www.diystompboxes.com/smfforum/index.php?topic=128573.0)[8](https://githubflow.github.io/)
