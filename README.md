# SNES Synth Controller

## Description

This project transforms a classic Super Nintendo (SNES) controller into a versatile musical synthesizer and MIDI controller. Using a Teensy microcontroller (tested on 3.2) and the Teensy Audio Shield, it allows real-time performance with both an internal synth engine and external MIDI devices.

The goal is to provide an intuitive and fun interface for musical expression, leveraging the familiar layout of the SNES controller for triggering notes, chords, rhythmic patterns, and controlling parameters.

## Hardware Required

*   **Teensy 3.x/4.x:** (Developed on Teensy 3.2, likely compatible with others).
*   **Teensy Audio Shield:** For audio input/output and processing.
*   **SNES Controller:** A standard Super Nintendo controller.
*   **SNES Controller Adapter/Wiring:** Appropriate connection from the SNES controller plug to the Teensy's digital pins (check `controller.h` for specific pin definitions: `SNES_DATA`, `SNES_CLOCK`, `SNES_LATCH`).
*   Standard USB cable for Teensy programming and MIDI communication.
*   (Optional) MIDI interface/device for using the MIDI output features or receiving external MIDI clock.
*   (Optional) Audio connection (headphones/speakers) for the internal synth.

## Features

*   **Multiple Play Modes:**
    *   **Monophonic Mode:** Plays one note at a time with last-note priority, based on the selected scale.
    *   **Chord Button Mode:** Each primary button triggers a pre-defined chord based on the current scale.
    *   **Boogie Mode:** A rhythmic mode synchronized to an external MIDI clock or using a remembered tempo.
        *   Plays repeating 8th notes based on held buttons (highest priority = most recently pressed).
        *   Adjustable **Swing** amount (0% to 100% triplet feel).
        *   L/R buttons mute/skip the first/second 8th note respectively.
        *   Holding L+R together switches to playing straight 8th note **Triplets**.
        *   **MIDI Clock Sync:** Locks tempo using a sample-and-hold mechanism after receiving MIDI Start and the first 24 clock ticks.
        *   **Internal Trigger:** If MIDI clock stops after tempo lock, the mode continues using the remembered tempo, with button presses defining the rhythmic start ("the one").
        *   **Monophonic Fallback:** Acts like Monophonic mode if selected *before* MIDI clock starts and locks tempo.
*   **Multiple Note Profiles:**
    *   **Scale Profile:** Maps buttons to degrees of the currently selected scale (Major, Minor, etc.).
    *   **Thunderstruck Profile:** Custom mapping for playing the "Thunderstruck" intro riff.
*   **Internal Synthesizer:** Basic synth voices provided by the Teensy Audio library.
*   **MIDI Output:** Sends MIDI Note On/Off messages via USB MIDI, allowing control of external synths or DAWs.
*   **Scale & Key Control:**
    *   Selectable musical scales (Major, Minor, Harmonic Minor, Melodic Minor, Lydian, Mixolydian, Dorian available).
    *   Adjustable base note (root).
    *   Adjustable key offset (transpose).
    *   Control via Serial commands.
    *   **Scale Mapping:** When using the Scale Profile:
        *   If the scale has *fewer* notes than available buttons (typically 7 notes vs 10 buttons), the mapping loops through the scale notes into the next octave for the remaining buttons.
        *   If the scale has *more* notes than available buttons (e.g., Chromatic=12), only the first N notes corresponding to the buttons are used.
*   **Performance Controls (Button Combos - Hold L+R then press...):**
    *   **A:** Toggle Portamento On/Off.
    *   **Up:** Switch Play Style (Mono/Chord - NOTE: Does not cycle Boogie).
    *   **B:** Cycle through Waveforms (Sine, Saw, Square, Triangle).
    *   **X:** Cycle Vibrato Depth (Off, Low, Medium, High).
    *   **Y:** Cycle Vibrato Rate (Off, 5Hz, 10Hz).
    *   **Select:** Toggle between Scale Mapping Profile and Thunderstruck Profile.
    *   **Start:** Toggle Boogie Mode On/Off.
*   **Pitch Bend:** L/R buttons shift pitch down/up (-12/+12 semitones) when *not* in Boogie mode.
*   **Serial Command Interface:** Control parameters via the Arduino Serial Monitor or a separate control application (see Usage).
*   **Debug Output:** Provides status information via the Serial Monitor.

## Code Structure

A brief overview of the key source files:

*   **`main.ino`:** Main sketch file containing `setup()`, `loop()`, MIDI callback handlers (`handleClock`, `handleStart`, `handleStop`, etc.), command processing loop, and top-level mode switching logic.
*   **`synth_state.h`:** Defines the main `SynthState` struct, holding all global state variables for the synthesizer (modes, button states, MIDI info, timing, etc.) and important constants/enums.
*   **`controller.h/.cpp`:** Handles reading input from the SNES controller (debouncing, detecting presses/releases).
*   **`audio.h/.cpp`:** Manages the Teensy Audio library setup, synth voice configuration, `playNote`, `stopNote`, portamento, vibrato, and potentially `getBaseMidiNote`.
*   **`playstyles.h/.cpp`:** Implements the core logic for each play mode (`handleMonophonic`, `handleChordButton`, `handleBoogieTiming`). Contains note mappings (`buttonToMusicalPosition`, `thunderstruckMidiNotes`).
*   **`commands.h/.cpp`:** Handles parsing and executing commands received via the Serial interface.
*   **`synth.h/.cpp`:** Contains scale definitions (`SCALE_DEFINITIONS`) and the `updateScale` function. May contain other general synth utility functions.
*   **`debug.h/.cpp`:** Provides macros and functions for categorized debug logging (`DEBUG_INFO`, `DEBUG_DEBUG`, etc.).
*   **`utils.h/.cpp`:** (If exists) Likely contains general utility functions used across the project.
*   **`chords.h/.cpp`:** (If exists) Likely defines chord structures and logic for `handleChordButton`.

## Setup & Installation

1.  **Arduino IDE:** Install the latest Arduino IDE ([arduino.cc](https://www.arduino.cc/en/software)).
2.  **Teensyduino:** Install the Teensyduino add-on for the Arduino IDE ([pjrc.com/teensy/td_download.html](https://www.pjrc.com/teensy/td_download.html)), ensuring support for your Teensy model is selected.
3.  **Libraries:** The project relies heavily on the built-in Teensy Audio Library and MIDI library, installed with Teensyduino. No external library installations should be required.
4.  **Hardware Connection:** Connect the SNES controller pins to the Teensy according to the definitions in `controller.h`. Connect the Audio Shield.
5.  **Upload Code:** Open `main.ino` in the Arduino IDE, select your Teensy model and "Serial + MIDI" from the Tools menu, select the Port, and click Upload.

## Usage

1.  **Connect:** Connect the Teensy via USB. Connect audio output or use USB MIDI.
2.  **Serial Monitor:** Open the Arduino Serial Monitor (**Baud Rate: 115200**) to view status and send commands.
3.  **Playing:**
    *   **Buttons:** Trigger notes/chords/Boogie notes based on mode.
    *   **L/R Combos:** Hold L+R then press another button for functions (see Features list).
    *   **Boogie Mode:** Toggle with L+R+Start.
        *   Requires external MIDI Clock Start signal to lock tempo.
        *   Plays monophonically before tempo lock.
        *   Press/hold note buttons to play rhythm.
        *   Hold L/R to mute/skip downbeat/upbeat 8ths.
        *   Hold L+R together for triplets.
        *   Stops when MIDI Stop received or clock times out.
        *   Remembers tempo for Internal Trigger mode after clock stops (button press starts rhythm).
4.  **Serial Commands:** (Type and press Enter)
    *   `set mode <0-6>` (e.g., `set mode 1` for Natural Minor)
    *   `set base <MIDI#>` (e.g., `set base 60` for C4)
    *   `set key <0-11>` (e.g., `set key 0` for C)
    *   `set swing <0.0-1.0>` (e.g., `set swing 0.5`)
    *   `mono` / `chord` (Note: Does not affect Boogie mode selection)
    *   `portamento` (Toggles)
    *   `boogie` (Toggles Boogie Mode)
    *   `rhythmic` (Toggles Rhythmic Mode - if implemented)
    *   `profile` (Toggles Scale/Thunderstruck)
    *   `waveform <0-3>`
    *   `vibdepth <0-3>`
    *   `vibrate <0-2>`
    *   `debug <CAT> <LEVEL>` (See `debug.h`)
    *   `help` (Show available commands - *Needs implementation in commands.cpp*)

## Future Plans / Roadmap

*   Implement **Polyphonic Mode**.
*   Implement **Arpeggio Mode**.
*   Integrate **Audio Effects**.
*   Refine synth voices.
*   Add **EEPROM Saving**.
*   Refine `help` command.

## Contributing

Contributions, issues, and feature requests are welcome.

## License

*(Consider adding a license)* 