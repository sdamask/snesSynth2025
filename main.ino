// main.ino
// Main entry point for the SNES synthesizer project. Initializes the synthesizer,
// handles the main loop, processes MIDI and Serial commands, and updates the state.

#include "synth_state.h"
#include "controller.h"
#include "utils.h"
#include "audio.h"
#include "synth.h"
#include "commands.h"
#include "debug.h"
#include "playstyles.h"

SynthState state;
unsigned long lastPrintTime = 0;
const unsigned long printCooldown = 50;  // 50ms cooldown between prints
unsigned long lastHeldChangeTime = 0;
const unsigned long debouncePeriod = 20;  // 20ms debounce period
bool pendingPrint = false;

void setup() {
    setupDebug(9600);  // Initialize debug system first
    DEBUG_INFO(CAT_STATE, "Starting SNES Synthesizer initialization...");
    
    Serial.begin(9600);
    setupController();
    DEBUG_INFO(CAT_BUTTON, "Controller initialized");
    
    setupAudio();
    DEBUG_INFO(CAT_AUDIO, "Audio system initialized");
    
    // Initialize SynthState with default values
    initializeSynthState(state);
    DEBUG_INFO(CAT_STATE, "Synth state initialized");
    
    // Print initial state
    updateScale(state);
    DEBUG_INFO(CAT_SCALE, "Initial scale updated");
    printStatus(state);
    DEBUG_INFO(CAT_STATE, "Initialization complete");
}

void loop() {
    // Reset command flag at start of loop
    state.commandJustExecuted = false;

    // Read USB MIDI messages
    usbMIDI.read();

    // Check for Serial commands from Processing (GUI)
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        DEBUG_INFO(CAT_COMMAND, "Received command: %s", command.c_str());
        handleSerialCommand(command, state);
    }

    // Update button states
    buttonState(state);
    
    // Check for commands (scale changes, portamento toggle, etc.)
    checkCommands(state);
    
    // Update scale if needed
    if (state.needsScaleUpdate) {
        updateScale(state);
        state.needsScaleUpdate = false;
    }
    
    // Update audio system (handle portamento)
    updateAudio(state);
    
    // Call the appropriate playstyle function ONLY if a command wasn't just executed
    if (!state.commandJustExecuted) {
        switch (state.playStyle) {
            case MONOPHONIC:
                handleMonophonic(state);
                break;
            case CHORD_BUTTON:
                handleChordButton(state);
                break;
            case POLYPHONIC:
                handlePolyphonic(state);
                break;
        }
    } // else: Skip playstyle handler this cycle because a command took priority

    // Check for changes in held state (only for note buttons 0-9)
    bool buttonPressedThisFrame = false; // Flag specifically for press events
    for (int i = 0; i < MAX_NOTE_BUTTONS; i++) { // Check only 0-9
        if (state.pressed[i]) { // Check the .pressed array directly
            buttonPressedThisFrame = true;
            break;
        }
    }

    unsigned long currentTime = millis();

    // If a button was pressed, start the debounce/cooldown timer for printing
    if (buttonPressedThisFrame) {
        lastHeldChangeTime = currentTime; // Use the same timer variable for simplicity
        pendingPrint = true;
        DEBUG_DEBUG(CAT_BUTTON, "Button pressed, print pending");
    }

    // If enough time has passed since the last PRESS and there's a pending print
    if (pendingPrint && (currentTime - lastHeldChangeTime >= debouncePeriod)) {
        // Only print if enough time has passed since the last print (cooldown)
        if (currentTime - lastPrintTime >= printCooldown) {
            printStatus(state);
            lastPrintTime = currentTime;
            DEBUG_DEBUG(CAT_STATE, "State printed after debounce");
        }
        pendingPrint = false;  // Reset pending print
    }

    // Update prevHeld for the next iteration
    for (int i = 0; i < MAX_NOTE_BUTTONS; i++) {
        state.prevHeld[i] = state.held[i];
    }

    // No delay in the main loop - let the audio system run as fast as possible
    yield();  // Allow other tasks to run if needed
}

void OnNoteOn(byte channel, byte note, byte velocity) {
    // To be implemented later
}

void OnControlChange(byte channel, byte control, byte value) {
    if (channel == 1) {  // Only respond to channel 1
        switch (control) {
            case 20:  // CC 20: Change scale mode
                state.scaleMode = map(value, 0, 127, 0, 6);  // 7 scales (0-6)
                state.needsScaleUpdate = true;  // Mark for update
                Serial.print("Scale Mode Changed to: ");
                Serial.println(state.scaleMode);
                break;
            case 21:  // CC 21: Change base note
                state.baseNote = map(value, 0, 127, 36, 84);  // Map to 3 octaves around middle C
                state.needsScaleUpdate = true;  // Mark for update
                Serial.print("Base Note Changed to: ");
                Serial.println(state.baseNote);
                break;
            case 22:  // CC 22: Change keyOffset
                state.keyOffset = map(value, 0, 127, 0, 11);  // Map 0-127 to 0-11 semitones
                state.needsScaleUpdate = true;  // Mark for update
                Serial.print("Key Offset Changed to: ");
                Serial.println(state.keyOffset);
                break;
        }
    }
}