# Project Roadmap: SNES Synth Extensibility & Testing

This document outlines the planned steps to enhance the SNES Synth codebase for better extensibility (controllers, music theory) and implement a robust testing methodology.

## Phase 1: Foundation & Controller Abstraction

- [x] **Commit Current Stable State:** Ensure the latest working code (including Sample/Hold sync, Swing/Triplet Boogie mode) is committed and pushed to the `main` branch. (Completed: Commit `2e3ccb2`)
- [ ] **Create Development Branch:** Create a new branch (e.g., `develop` or `feature/extensibility`) from `main` for major refactoring work.
  ```bash
  git checkout main
  git pull
  git checkout -b develop
  git push -u origin develop
  ```
- [ ] **Define Generic Input Struct:** Design and define a `ControllerInput` struct in a new header (e.g., `input_abstraction.h`) containing logical button/axis states (e.g., `bool action1Pressed`, `bool dpadUpHeld`, `float analogX`, etc.).
- [ ] **Create SNES Driver:** Refactor `controller.cpp` into an `snes_driver.cpp`. Modify its primary function (e.g., `readSnesController`) to populate the generic `ControllerInput` struct instead of directly modifying `SynthState` button arrays.
- [ ] **Update Main Loop:** Modify `main.ino` to include `input_abstraction.h`, create an instance of `ControllerInput`, and call the SNES driver function to fill it in each loop.
- [ ] **Refactor State/Playstyles:** Update `SynthState` (if needed) and modify `main.ino`, `playstyles.cpp`, `commands.cpp` etc. to read button states from the `ControllerInput` struct instance using the *logical* names, instead of directly using `state.held[BTN_Y]` etc. This is a significant refactoring step.
- [ ] **Testing Framework - Setup:** Research and select a testing approach suitable for this embedded project. Given the need for interactive confirmation, the proposed guided serial test seems appropriate. Design the basic structure for this test mode.
- [ ] **Testing Framework - Initial Tests:** Implement initial tests within the framework focusing *only* on verifying the core SNES controller input and basic Monophonic output after the abstraction changes. Cover all buttons and directions.

## Phase 2: Music Theory Flexibility

*(Prerequisite: Phase 1 completed and merged/stable)*

- [ ] **Refine Scale Handling:** Modify `updateScale` function (in `synth.cpp`) to correctly handle scales of varying lengths based on the `-1` terminator in `SCALE_DEFINITIONS`. Store the actual `scaleLength` in `SynthState` after calculation.
- [ ] **Refine `buttonToMusicalPosition`:** Analyze and refactor the mapping from the 10 primary buttons to scale degrees. Consider:
    - Option A: Make the current looping behavior explicit and configurable (e.g., a `bool scaleLoops` flag).
    - Option B: Introduce different mapping profiles alongside scale modes.
    - Option C: Allow direct button-to-note or button-to-interval mapping via commands.
- [ ] **Add Chromatic Support:** Implement a method for chromatic playing. This could be:
    - A specific `scaleMode` that causes `updateScale` to fill `scaleHolder` chromatically.
    - A new `PROFILE_CHROMATIC` for `customProfileIndex` that bypasses `scaleHolder` in playstyles.
- [ ] **Testing Framework - Music Theory Tests:** Add guided tests for:
    - Setting different scale modes (`set mode`) and verifying output notes correspond to the selected scale.
    - Testing base note and key offset changes.
    - Verifying the defined behavior for scales with <10 and >10 notes.
    - Testing chromatic output (if implemented).

## Phase 3: Controller Agnosticism & Expansion

*(Prerequisite: Phase 1 completed and merged/stable)*

- [ ] **Add USB HID Driver:** Create a `usb_hid_driver.cpp` that reads a standard USB gamepad/joystick and populates the *same* generic `ControllerInput` struct used by the SNES driver.
- [ ] **Add MIDI Input Driver:** Create a `midi_input_driver.cpp` that interprets incoming MIDI Note On/Off messages to populate the `ControllerInput` struct (mapping specific MIDI notes to logical buttons).
- [ ] **Driver Selection Mechanism:** Implement a way to choose the active controller driver (e.g., via a `#define` at compile time, a serial command, or maybe auto-detection if possible).
- [ ] **Mapping Configuration:** (Optional but recommended) Implement commands or a system to configure how the physical inputs of the *selected* controller map to the *logical* inputs used by the synth engine.
- [ ] **Testing Framework - Controller Tests:** Add guided tests for verifying input from different controller types (USB HID, MIDI Input) once implemented.

## Phase 4: Advanced Features & Refinements

- [ ] **Mode Management Refactor:** Refactor mode switching using a state machine or function pointer array as discussed.
- [ ] **Implement Polyphonic/Arp Modes:** Add the planned playstyles.
- [ ] **Implement Audio Effects:** Integrate effects.
- [ ] **Implement EEPROM Saving:** Persist settings.
- [ ] **Improve Serial Commands:** Add `help` command, refine existing commands.
- [ ] **Testing Framework - Feature Tests:** Add guided tests for new modes (Poly, Arp), effects, and EEPROM saving/loading.

## Continuous Improvement

- [ ] Code Cleanup & Refactoring
- [ ] Documentation Updates (Code comments, README)
- [ ] Performance Optimization (if needed)
