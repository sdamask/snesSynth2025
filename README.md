# SNES Synth Controller

## Description

This project transforms a classic Super Nintendo (SNES) controller into a versatile musical synthesizer and MIDI controller. Using a Teensy 3.2 microcontroller and the Teensy Audio Shield, it allows real-time performance with both an internal synth engine and external MIDI devices.

The goal is to provide an intuitive and fun interface for musical expression, leveraging the familiar layout of the SNES controller for triggering notes, chords, and controlling parameters.

## Hardware Required

*   **Teensy 3.2:** The core microcontroller.
*   **Teensy Audio Shield:** For audio input/output and processing.
*   **SNES Controller:** A standard Super Nintendo controller.
*   **SNES Controller Adapter/Wiring:** Appropriate connection from the SNES controller plug to the Teensy's digital pins (check `controller.h` for specific pin definitions: `SNES_DATA`, `SNES_CLOCK`, `SNES_LATCH`).
*   Standard USB cable for Teensy programming and MIDI communication.
*   (Optional) MIDI interface/device for using the MIDI output features.
*   (Optional) Audio connection (headphones/speakers) for the internal synth.

## Features

*   **Multi-Mode Play:**
    *   **Monophonic Mode:** Plays one note at a time with last-note priority.
    *   **Chord Button Mode:** Each primary button triggers a pre-defined chord based on the current scale.
*   **Internal Synthesizer:** Basic synth voices provided by the Teensy Audio library.
*   **MIDI Output:** Sends MIDI Note On/Off messages via USB MIDI, allowing control of external synths or DAWs.
*   **Scale & Key Control:**
    *   Selectable musical scales (Major, Minor, etc.).
    *   Adjustable base note (root).
    *   Adjustable key offset (transpose).
    *   Control via MIDI CC messages or Serial commands.
*   **Performance Controls:**
    *   **Octave Shift:** L/R buttons shift the octave up/down (-12/+12 semitones) in Monophonic and Chord modes.
    *   **Portamento:** Smooth pitch glide between notes, toggleable via L+R+A button combo.
    *   **Mode Switching:** Cycle between Monophonic and Chord modes via L+R+Up button combo.
*   **Serial Command Interface:** Control parameters like scale, key, base note, play mode, and portamento via the Arduino Serial Monitor or a separate control application.
*   **Debug Output:** Provides status information via the Serial Monitor.

## Current Status

*   Core Monophonic and Chord modes are functional.
*   Note priority, portamento, octave control, and mode switching via button combos are working.
*   MIDI output and Serial/MIDI CC input for parameters are implemented.
*   Codebase is modular and organized.

## Setup & Installation

1.  **Arduino IDE:** Install the latest Arduino IDE ([arduino.cc](https://www.arduino.cc/en/software)).
2.  **Teensyduino:** Install the Teensyduino add-on for the Arduino IDE ([pjrc.com/teensy/td_download.html](https://www.pjrc.com/teensy/td_download.html)), ensuring support for Teensy 3.2 is selected during installation.
3.  **Libraries:** The project relies heavily on the built-in Teensy Audio Library and MIDI library, which are installed with Teensyduino. No external library installations should be required.
4.  **Hardware Connection:** Connect the SNES controller pins to the Teensy according to the definitions in `controller.h`. Connect the Audio Shield to the Teensy.
5.  **Upload Code:** Open `main.ino` in the Arduino IDE, select "Teensy 3.2" and the correct USB type (likely "Serial + MIDI") from the Tools menu, select the appropriate Port, and click Upload.

## Usage

1.  **Connect:** Connect the Teensy via USB to your computer. Connect headphones/speakers to the Audio Shield or connect a MIDI device/interface to your computer to receive MIDI from the Teensy.
2.  **Serial Monitor:** Open the Arduino IDE's Serial Monitor (set to 9600 baud) to view status updates and send commands.
3.  **Playing:**
    *   **Buttons:** The primary buttons trigger notes or chords based on the selected mode and scale (Default order: Down, Left, Up, Right, Select, Start, Y, B, X, A corresponds to scale degrees 1-10).
    *   **L/R:** Hold for octave down/up.
    *   **L+R+A:** Press A while holding L+R to toggle Portamento On/Off. A confirmation message appears in the Serial Monitor.
    *   **L+R+Up:** Press Up while holding L+R to switch between Monophonic and Chord Button modes. A confirmation message appears.
    *   **L+R+B:** Press B while holding L+R to cycle through Waveforms (Sine, Saw, Square, Triangle).
    *   **L+R+X:** Press X while holding L+R to cycle Vibrato Depth (Off, Low, Medium, High).
    *   **L+R+Y:** Press Y while holding L+R to cycle Vibrato Rate (Off, 5Hz, 10Hz).
4.  **Serial Commands:** Type commands into the Serial Monitor input and press Enter (examples):
    *   `scale <0-6>` (e.g., `scale 1` for Natural Minor)
    *   `base <36-84>` (e.g., `base 48` for C3)
    *   `key <0-11>` (e.g., `key 2` for D)
    *   `mono`
    *   `chord`
    *   `portamento` (toggles)

## Future Plans / Roadmap

*   Implement **Polyphonic Mode**.
*   Implement **Arpeggio Mode** with controls for direction, rate, etc.
*   Integrate **Audio Effects** from the Teensy Audio Library (Reverb, Delay, Filter) with button controls.
*   Implement **MIDI Note Input Learn Mode** to allow user-mapping of buttons to custom chords via an external MIDI keyboard.
*   Add **EEPROM/Flash Saving** for persisting settings (scale, key, mode, etc.) between power cycles.
*   Refine synth voices and parameters.
*   Add hardware feedback (LEDs, small display).

## Contributing

Contributions, issues, and feature requests are welcome. Please feel free to open an issue or submit a pull request.

## License

*(Consider adding an open-source license like MIT or GPL if you wish)* 