# Real-Time Synthesizer Overhaul (Zephyr RTOS)

This project takes a provided microcontroller synthesizer that was implemented as a **superloop** and redesigns it into a **real-time Zephyr-based system** that produces clear audio (no clipping/clicking) under normal load.

## Problem
The original implementation executes four tasks sequentially in a fixed loop:
1) peripherals (switches/encoders), 2) keyboard input, 3) audio synthesis, 4) audio output (I2S/DMA).  
This caused missed deadlines and audible artifacts because tasks have different timing/jitter requirements.

## What we did
### 1) Measured timing + CPU utilization
- Used a logic analyzer to measure **AECT/WCET** of each task and derived processor utilization for average and worst-case use cases. 
- Fixed measurement methodology by moving LED toggles inside each task so timing reflects true task execution (not superloop “leftover time”).

### 2) Replaced the superloop with Zephyr threads
- Split the system into **three threads**: combined (Task 1+2), Task 3 (synthesis), Task 4 (audio out), with priorities chosen to protect I2S deadlines.
- Chosen periods (example): peripherals 10 ms, keyboard 50 ms, synthesis 50 ms, audio out 50 ms, justified via WCET + audio frame constraints.
- Increased tick resolution by setting `CONFIG_SYS_CLOCK_TICKS_PER_SEC=25000` to improve timing precision for Task 4. 

### 3) Identified race conditions
- Documented data races (keys array, encoder state, audio buffer) and justified which were tolerable vs required fixes.

### 4) Implemented double buffering for audio (no clicks)
- Fixed the core “reader/writer” race between synthesis, audio thread, and DMA using:
  - `k_mem_slab` with **two fixed-size blocks**
  - `k_msgq` to pass full buffers from Task 3 → Task 4
  - I2S priming: PREPARE → queue two blocks → START; DROP on failure

### 5) Overload detection + safe degradation
- Implemented overload handling where synthesis detects it is close to deadline, stops computing the current frame, zeros the remainder, and flags overload for the next frame (LED indicator).

### 6) Interrupt-driven peripherals + debouncing (Part IV)
- Moved switch updates from polling to GPIO interrupts.
- Implemented debouncing using `k_work_reschedule()` (5 ms) and forwarded events via a message queue to the peripherals thread.
- Bonus: interrupt-based encoder support via the I2C expander IRQ line (PB4), with a short debounce guard (~200 µs based on tick time).

## Results
- Threaded design reduced synthesis WCET under typical use and improved responsiveness when decreasing block period (e.g., 50 ms → 30 ms).
- Double buffering eliminated audio corruption under normal operation by preventing DMA and synthesis from touching the same buffer.
- Overload mode prevents undefined audio output by zero-filling unfinished samples and visibly reporting overload.

## Repository notes
- Main implementation is on the `main` branch.
- Interrupt-based implementation (Part IV + bonus) is in the `bonus` branch (commit hash in the report).
