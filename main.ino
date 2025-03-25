// main.ino
// Main entry point for the SNES synthesizer project. Initializes the synthesizer,
// handles the main loop, processes MIDI and Serial commands, and updates the state.

#include "synth_state.h"
#include "controller.h"
#include "utils.h"
#include "audio.h"
#include "synth.h"
#include "commands.h"
#include "playstyles.h"

SynthState state;
unsigned long lastPrintTime = 0;
const unsigned long printCooldown = 200;  // 200ms cooldown between prints
unsigned long lastHeldChangeTime = 0;
const unsigned long debouncePeriod = 50;  // 50ms debounce period
bool pendingPrint = false;

void setup() {
    Serial.begin(9600);
    setupController();
    setupAudio();
    // Initialize SynthState with default values
    initializeSynthState(state);
    // Set up USB MIDI
    usbMIDI.setHandleNoteOn(OnNoteOn);
    usbMIDI.setHandleControlChange(OnControlChange);
    // Print initial state
    updateScale(state);
    printStatus(state);
}

void loop() {
    // Read USB MIDI messages
    usbMIDI.read();

    // Check for Serial commands from Processing (GUI)
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        handleSerialCommand(command, state);
    }

    buttonState(state);
    checkCommands(state);
    updateScale(state);

    // Call the appropriate playstyle function
    switch (state.playStyle) {
        case MONOPHONIC:
            handleMonophonicPlayStyle(state);
            break;
        case CHORD_BUTTON:
            handleChordButtonPlayStyle(state);
            break;
        case POLYPHONIC:
            // To be implemented later
            break;
    }

    // Check for changes in held state
    bool heldChanged = false;
    for (int i = 0; i <= 11; i++) {
        if (state.held[i] != state.prevHeld[i]) {
            heldChanged = true;
            break;
        }
    }

    unsigned long currentTime = millis();

    // If held state changed, start the debounce timer
    if (heldChanged) {
        lastHeldChangeTime = currentTime;
        pendingPrint = true;
    }

    // If enough time has passed since the last change and there's a pending print
    if (pendingPrint && (currentTime - lastHeldChangeTime >= debouncePeriod)) {
        // Only print if enough time has passed since the last print (cooldown)
        if (currentTime - lastPrintTime >= printCooldown) {
            printStatus(state);
            lastPrintTime = currentTime;
        }
        pendingPrint = false;  // Reset pending print
    }

    // Update prevHeld for the next iteration
    for (int i = 0; i <= 11; i++) {
        state.prevHeld[i] = state.held[i];
    }

    delay(10);
}

void OnNoteOn(byte channel, byte note, byte velocity) {
    // To be implemented later
}

void OnControlChange(byte channel, byte control, byte value) {
    if (channel == 1) {  // Only handle CC messages on channel 1
        switch (control) {
            case 20:  // CC 20: Change scaleMode
                state.scaleMode = map(value, 0, 127, 0, NUM_SCALES - 1);  // Map 0-127 to 0-(NUM_SCALES-1)
                Serial.print("Scale Mode Changed to: ");
                Serial.println(state.scaleMode);
                updateScale(state);  // Update the scale immediately
                break;
            case 21:  // CC 21: Change baseNote
                state.baseNote = map(value, 0, 127, 0, 127);  // Map 0-127 to MIDI note range
                Serial.print("Base Note Changed to: ");
                Serial.println(state.baseNote);
                updateScale(state);  // Update the scale immediately
                break;
            case 22:  // CC 22: Change keyOffset
                state.keyOffset = map(value, 0, 127, 0, 11);  // Map 0-127 to 0-11 semitones
                Serial.print("Key Offset Changed to: ");
                Serial.println(state.keyOffset);
                updateScale(state);  // Update the scale immediately
                break;
        }
    }
}

void handleSerialCommand(String command, SynthState& state) {
    // To be implemented later for GUI control
}