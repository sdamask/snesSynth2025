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

// Forward declarations
void handleSerialCommand(String command, SynthState& state);
void processMidiTick(SynthState& state); // Renamed from handleMidiClock
void handleClock();
void handleStart();
void handleStop();

void setup() {
    // Initialize Serial communication
    setupDebug(9600); 

    // Initialize the SNES controller
    setupController();
    DEBUG_INFO(CAT_CONTROLLER, "Controller initialized");

    // Initialize audio system
    setupAudio();
    DEBUG_INFO(CAT_AUDIO, "Audio system initialized");

    // Initialize synth state
    initializeSynthState(state);
    DEBUG_INFO(CAT_STATE, "Synth state initialized");

    // Ensure the initial scale is calculated and ready
    updateScale(state);
    DEBUG_INFO(CAT_STATE, "Initial scale updated");

    // Setup MIDI handlers
    usbMIDI.setHandleNoteOn(OnNoteOn);
    usbMIDI.setHandleControlChange(OnControlChange); // Ensure CC handler is set
    usbMIDI.setHandleClock(handleClock);
    usbMIDI.setHandleStart(handleStart);
    usbMIDI.setHandleStop(handleStop);
}

void loop() {
    // Reset command flag at start of loop
    state.commandJustExecuted = false;

    // Read USB MIDI messages - Calls handleClock, handleStart, handleStop internally
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

    unsigned long currentTime = millis();

    // If a command was executed this frame, flag that a print is pending
    if (state.commandJustExecuted) {
        lastHeldChangeTime = currentTime; // Use the same timer variable for simplicity
        pendingPrint = true;
        DEBUG_DEBUG(CAT_COMMAND, "Command executed, print pending");
    }

    // If enough time has passed since the last COMMAND and there's a pending print
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

// MIDI Clock/System Realtime Callbacks
void handleClock() {
    // Called by usbMIDI.read() 24 times per quarter note
    DEBUG_VERBOSE(CAT_MIDI, "MIDI Clock Tick Received");
    
    // Only process if sync is enabled (MIDI Start received)
    if (state.midiSyncEnabled) {
        // Increment our internal counter first
        state.midiClockCount = (state.midiClockCount + 1) % 24; // Increment and wrap at 24
        
        // Now process the logic based on this tick
        processMidiTick(state);
    } 
}

void handleStart() {
    DEBUG_INFO(CAT_MIDI, "MIDI Start Received");
    state.midiClockCount = 0;
    state.currentBeat = 0;
    state.lastQuarterNoteTime = millis();
    state.midiSyncEnabled = true; // Enable processing
}

void handleStop() {
    DEBUG_INFO(CAT_MIDI, "MIDI Stop Received");
    state.midiSyncEnabled = false; // Disable processing
    // Maybe stop boogie notes if needed
    if (state.lastBoogieMidiNote != -1) {
        sendMidiNoteOff(state.lastBoogieMidiNote, 0, MIDI_CHANNEL);
        stopNote(0); 
        state.lastBoogieMidiNote = -1;
    }
}

// MIDI Note/CC Handlers
void OnNoteOn(byte channel, byte note, byte velocity) {
    // Implement later if needed for external MIDI input
    DEBUG_DEBUG(CAT_MIDI, "MIDI Note On: Chan=%d Note=%d Vel=%d", channel, note, velocity);
}

void OnControlChange(byte channel, byte control, byte value) {
    // Implement later if needed
    DEBUG_DEBUG(CAT_MIDI, "MIDI CC: Chan=%d Ctrl=%d Val=%d", channel, control, value);
}