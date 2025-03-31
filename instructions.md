# SNES Synth Controller - Development Plan

## Project Goal

To create a musical instrument using a Super Nintendo controller connected to a Teensy 3.2 with an Audio Shield. The instrument should:
1.  Act as a standalone synthesizer using the Teensy Audio library.
2.  Output MIDI notes/chords to control external synthesizers.
3.  Support multiple play modes (Monophonic, Chord, Polyphonic, Arpeggio).
4.  Allow control over parameters like octave, portamento, and potentially audio effects via button combinations.
5.  Initially accept MIDI CC for some parameter changes, with future plans for full MIDI Note In for custom chord mapping.

## Current Status

The basic code structure is in place, but the core functionality (translating SNES button presses into sound and MIDI) is reportedly not working correctly.

## Phase 1: Debugging Core Functionality

**(Goal: Get the basic Monophonic and Chord modes working reliably with sound output, MIDI output, and essential controls.)**

1.  **Verify Controller Input:**
    *   Ensure the Teensy is correctly reading button presses/releases from the SNES controller.
    *   Use Serial Monitor debug messages (`debug.cpp`) to confirm button states are registered as expected in `controller.cpp` and reflected in the `SynthState`.
2.  **Verify Basic Audio Output (Monophonic):**
    *   In `MONOPHONIC` mode, confirm that pressing a designated note button triggers a sound from the Teensy Audio Shield.
    *   Debug `synth.cpp` and `audio.cpp` to ensure the audio engine is initialized and responding to note on/off commands triggered by the controller state.
3.  **Verify Basic MIDI Output (Monophonic):**
    *   Confirm that pressing a note button sends the correct MIDI Note On message via USB MIDI.
    *   Confirm releasing the button sends the corresponding MIDI Note Off message.
    *   Check `playstyles.cpp` (specifically `handleMonophonic`) and `midi.cpp` / `midi_utils.cpp`.
4.  **Verify Chord Mode Audio/MIDI:**
    *   Switch to `CHORD_BUTTON` mode (using the L+R+Up combo or Serial command).
    *   Confirm pressing a button triggers the corresponding pre-defined chord notes both audibly and via MIDI output.
    *   Debug `handleChordButton` in `playstyles.cpp` and `chords.cpp`.
5.  **Verify Octave Control (L/R Buttons):**
    *   Check if L and R buttons correctly modify the `octaveModifier` in `SynthState` when in Monophonic or Chord mode.
    *   Confirm the resulting notes (audio and MIDI) reflect the octave shift. Debug `checkCommands` in `commands.cpp`.
6.  **Verify Portamento Toggle (L+R+A):**
    *   Check if the L+R+A combo correctly toggles the `portamentoEnabled` flag in `SynthState`.
    *   Confirm the audio output reflects the portamento change (requires checking `updateAudio` in `audio.cpp` and the synth voice implementation in `synth.cpp`).
7.  **Verify Mode Toggle (L+R+Up):**
    *   Confirm this combo correctly switches between `MONOPHONIC` and `CHORD_BUTTON` modes (or cycles through available modes). Debug `checkCommands` in `commands.cpp`.

**--> GIT COMMIT: "Core functionality (Mono/Chord modes, MIDI/Audio out, Octave, Portamento) working."**

## Phase 2: Implementing New Features

**(Goal: Add the planned Polyphonic and Arpeggio modes, effects, and enhanced MIDI capabilities.)**

1.  **Implement Polyphonic Mode:**
    *   Modify `playstyles.cpp` to add `handlePolyphonic`.
    *   Update `SynthState` if necessary to track multiple active notes.
    *   Ensure `synth.cpp`/`audio.cpp` can handle playing multiple voices simultaneously.
    *   Ensure correct MIDI Note On/Off messages are sent for multiple simultaneous presses.
    *   Test octave and portamento controls in this mode.
    **--> GIT COMMIT: "Implemented basic Polyphonic play mode."**
2.  **Implement Basic Arpeggio Mode:**
    *   Add `ARPEGGIO` to the `PlayStyle` enum.
    *   Modify `playstyles.cpp` to add `handleArpeggio`.
    *   Implement logic to cycle through notes derived from currently held buttons (either single notes in Poly mode or chord notes in Chord mode).
    *   Use a timer (e.g., `elapsedMillis`) to control the arpeggio rate.
    *   Output arpeggiated notes via audio and MIDI.
    **--> GIT COMMIT: "Implemented basic Arpeggio play mode (upward direction)."**
3.  **Implement Arpeggio Controls:**
    *   Assign L/R buttons new functions in Arpeggio mode (e.g., L=Pause/Resume, R=Change Direction - Up, Down, Up/Down, Random).
    *   Update `handleArpeggio` and potentially `checkCommands` to handle these controls.
    **--> GIT COMMIT: "Implemented Arpeggio controls (Pause, Direction)."**
4.  **Implement Audio Effects:**
    *   Explore available effects in the Teensy Audio library (Reverb, Delay, Chorus, Filter etc.).
    *   Modify the audio graph in `audio.cpp` to include desired effects.
    *   Add parameters to `SynthState` to control effects (e.g., `reverbLevel`, `delayTime`).
    *   Assign button combinations in `commands.cpp` to toggle effects or adjust their parameters.
    **--> GIT COMMIT: "Added basic audio effects (e.g., Reverb, Delay) with controls."**
5.  **Implement Full MIDI Note Input (Chord Mapping):**
    *   Implement the `OnNoteOn` and `OnNoteOff` MIDI callback functions in `main.ino` (or delegate to `midi.cpp`).
    *   Define a mechanism (maybe triggered by a specific mode or button combo) to enter a "MIDI learn" state.
    *   While in learn mode, map incoming MIDI notes to specific SNES controller buttons.
    *   Store these mappings (potentially in EEPROM for persistence).
    *   Modify `handleChordButton` to use these user-defined mappings instead of, or in addition to, the hardcoded chords.
    **--> GIT COMMIT: "Implemented MIDI Note Input for custom button-to-chord mapping."**

## Phase 3: Refinement and Testing

*   Thoroughly test all modes and features.
*   Refine timings, responsiveness, and sound parameters.
*   Clean up code, add comments, improve debugging output.
*   Consider adding hardware feedback (LEDs/Display).
*   Final documentation updates.

**--> GIT COMMIT: "Project complete / Final refinements."** 