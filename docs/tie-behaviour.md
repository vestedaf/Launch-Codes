
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
