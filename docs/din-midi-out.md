
# DIN‑MIDI OUT (Serial1 @ 31,250 baud)

This project outputs MIDI messages to a **DIN‑5 jack** from Teensy 4.0 via **Serial1**.  
Teensy’s MIDI guidance: **MIDI OUT requires only 2 resistors**, while **MIDI IN requires an opto‑isolator** (if you add DIN‑IN later). [4](https://www.kieranreck.co.uk/MIDI-6N137-vs-6N138-vs-6N139/)

## UART & pins (Teensy 4.0)
- **Serial1 TX** = **pin 1** (connects to DIN pin 5 through the series resistor).  
- **Serial1** runs at **31,250 baud** for standard MIDI. [4](https://www.kieranreck.co.uk/MIDI-6N137-vs-6N138-vs-6N139/)

## Standard MIDI OUT current-loop (summary)
The classic MIDI OUT circuit uses:
- A **series resistor** from TX to **DIN pin 5**, and  
- A **pull-up resistor** from Vcc (3.3 V or 5 V) to **DIN pin 4**,  
- **DIN pin 2** → ground (shield).

> Many reference schematics show **≈220 Ω** for both resistors; values can vary by implementation (3.3 V vs 5 V). Verify against your device and the PJRC schematic for Teensy 4.x. [9](https://little-scale.blogspot.com/2020/05/teensy-36-basics-receiving-midi-clock.html)[10](https://www.pjrc.com/teensy/gui/)

### Why opto‑isolation is for **MIDI IN**
MIDI specifies isolation on the **receiver** to prevent ground loops; therefore **MIDI IN** requires an opto‑isolator (e.g., 6N137/6N138). MIDI OUT does **not** use an opto‑isolator. [10](https://www.pjrc.com/teensy/gui/)

## Library choice
This firmware uses the **FortySevenEffects MIDI library** on **Serial1** to format messages (Note On/Off, etc.) for DIN‑MIDI. [7](https://forum.pjrc.com/index.php?threads/simple-demo-for-class-compliant-midi-teensy-4-1.73778/)
``
