# Launch-Codes
A Teensy 4.0-Based Polyphonic Midi Chord-Step Sequencer

A USB‑MIDI–driven **polyphonic chord sequencer** for Teensy 4.0.  
It **captures chords** (the final set of notes you held before releasing them), then **places that chord on a step grid** driven by external **MIDI clock**.  
**Gate length is fixed at 50%** for concise stabs; **Tie** lets you sustain a chord across steps.

## Features
- **USB‑MIDI (device)** input for notes and **MIDI clock** (Start/Stop/Continue + 24 PPQN tick).  
- **Chord capture** with a short **aggregation window** (25 ms), preserving **per‑note velocities**.  
- **64‑step grid** (first release), with **per‑step enable** and **per‑step Tie**.  
- **Outputs**: USB‑MIDI and **DIN‑MIDI OUT** via Serial1 @ **31,250 baud**.  
- **Gate = 50%** (3 of 6 clock ticks per 1/16 step); **Tie** overrides gate to sustain across steps.

> MIDI beat clock sends **24 pulses per quarter note**; a sixteenth note = **6 pulses**. We advance one step every 6 ticks. [3](https://www.pjrc.com/teensy/td_uart.html)

---

## Hardware

- **Board**: Teensy **4.0** (smaller footprint; USB device only).  
- **DIN‑MIDI OUT**: Teensy **Serial1 TX (pin 1)** routed to the DIN‑5 connector using the **standard 2‑resistor** current‑loop output (see `docs/din-midi-out.md`). Teensy’s MIDI docs note **MIDI OUT requires only 2 resistors** and **MIDI IN requires an opto‑isolator** (if you add DIN‑IN later). [4](https://www.kieranreck.co.uk/MIDI-6N137-vs-6N138-vs-6N139/)

---

## Software setup

1. Install **Arduino IDE** and **Teensy support**.  
   - Select **Tools → USB Type → MIDI** to make Teensy a **class‑compliant USB‑MIDI device**.  
   - The Teensy USB‑MIDI API (`usbMIDI`) provides send/receive and **Clock/Start/Continue/Stop** callbacks. [5](https://www.diystompboxes.com/smfforum/index.php?topic=128573.0)[6](https://docs.github.com/en/get-started/git-basics/git-workflows)
2. (Optional) The **FortySevenEffects MIDI library** drives **Serial1** for DIN‑MIDI at **31,250 baud**. It’s commonly used on Teensy and supports standard message formatting. [7](https://forum.pjrc.com/index.php?threads/simple-demo-for-class-compliant-midi-teensy-4-1.73778/)

---

## Build & Upload

1. Open `firmware/MIDIChordSeq_Tie/MIDIChordSeq_Tie.ino`.  
2. **Board**: Teensy **4.0**.  
3. **Tools → USB Type**: **MIDI**.  
4. Upload. The device will enumerate as a USB‑MIDI endpoint.

---

## Connect & Play

- **Clock source**: Your DAW or other midi clock source set to send **MIDI clock** (24PPQN) to Teensy (over USB).  
- **Chord capture**: Play a chord into Teensy over USB‑MIDI; when all notes are released, the chord is captured.  
- **Sequencing**: On **Start**, the grid runs at **1/16** resolution (one step per 6 ticks). Steps marked enabled will fire the chord; steps marked **Tie** will **sustain** the chord across boundaries without retriggering.

---

## Serial controls (for proof‑of‑concept)

Open **Serial Monitor** at 115200 and type:
- `p` — toggle play/stop  
- `s<N>` — toggle **enabled** for step `<N>`  
- `t<N>` — toggle **Tie** for step `<N>` (sustain into next step)  
- `dump` — print enabled/tie map

---

## License

**MIT** — maximizes reuse and adaptation, including downstream projects (e.g., Phazerville/Ornament & Crime forks). See `LICENSE` at repo root.

---
