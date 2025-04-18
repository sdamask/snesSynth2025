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

// --- Constants ---
#define MIDI_CLOCK_TIMEOUT_MS 500 // Timeout in milliseconds

// --- Global State ---
SynthState state;
unsigned long lastPrintTime = 0;
const unsigned long printCooldown = 50;  // 50ms cooldown between prints
unsigned long lastHeldChangeTime = 0;
const unsigned long debouncePeriod = 20;  // 20ms debounce period
bool pendingPrint = false;

// Forward declarations
void handleSerialCommand(String command, SynthState& state);
// void processMidiTick(SynthState& state); // Removed
void handleClock();
void handleStart();
void handleStop();

// How aggressively to correct phase errors (0.0 to 1.0). Smaller values are smoother but slower.
// #define PHASE_CORRECTION_FACTOR 0.1f 

void setup() {
    Serial.begin(115200);
    // while (!Serial); // Optional: Wait for serial connection
    delay(500); // Short delay for stability

    // DEBUG_INIT(Serial); // Initialize debug output
    DEBUG_INFO(CAT_STATE, "\n--- SNES Synth Booting ---");

    // Initialize state defaults
    state.playStyle = MONOPHONIC;
    state.baseNote = 3; // Changed baseOctave to baseNote, assuming '3' maps correctly, adjust if needed
    state.firstEighthNoteDurationRatio = 0.5f; // Set 1st 8th duration to 50%
    state.secondEighthNoteDurationRatio = 0.5f; // Set 2nd 8th duration to 50%
    state.swingAmount = 1.0f; // Default to 30% swing for testing

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
    
    // --- Update Scale if Needed --- 
    if (state.needsScaleUpdate) {
        updateScale(state); // Update state.scaleHolder based on state.scaleMode
    }
    
    // --- Handle High-Resolution Rhythmic Timing --- 
    // Run if Rhythmic mode is ON and tempo is available (live clock OR established)
    if (state.rhythmicModeEnabled && (state.midiSyncEnabled || state.tempoEstablished)) {
        unsigned long nowMicros = micros();
        // If tempo was established but clock stopped, usPerMidiTick will hold the last valid value
        unsigned long cycleDurationMicros = (unsigned long)(state.currentRhythmPatternLengthTicks * state.usPerMidiTick);
        unsigned long elapsedMicrosInCycle = nowMicros - state.cycleStartTimeMicros;

        // Check for cycle rollover
        if (cycleDurationMicros > 0 && elapsedMicrosInCycle >= cycleDurationMicros) {
            // Calculate overshoot to maintain accuracy across cycles
            unsigned long overshootMicros = elapsedMicrosInCycle - cycleDurationMicros;
            state.cycleStartTimeMicros = nowMicros - overshootMicros; 
            elapsedMicrosInCycle = overshootMicros; // Use overshoot for first check in new cycle
            
            // Reset played flags for the new cycle
            for(int i = 0; i < state.numNotesInPattern; ++i) { 
                state.notePlayedInCycle[i] = false; 
            }
            DEBUG_VERBOSE(CAT_PLAYSTYLE, "Rhythmic Cycle Rollover. Start: %lu", state.cycleStartTimeMicros);
        }

        // Iterate through pattern notes to see if any should play
        for (int i = 0; i < state.numNotesInPattern; ++i) {
            if (!state.notePlayedInCycle[i]) { // Only check notes not yet played this cycle
                // Calculate target time in micros for this note
                unsigned long targetMicrosForNote = state.cycleStartTimeMicros + (unsigned long)(state.currentRhythmPatternTicks[i] * state.usPerMidiTick);

                if (nowMicros >= targetMicrosForNote) {
                    // Time to potentially trigger this note index 'i'
                    
                    // Determine if L or R should trigger this specific note index
                    // L triggers even notes (0, 2, 4...), R triggers odd notes (1, 3, 5...)
                    bool triggerL = ((i % 2) == 0) && state.boogieLActive;
                    bool triggerR = ((i % 2) != 0) && state.boogieRActive;

                    if (triggerL || triggerR) {
                        // --- Send Note Off for previous note --- 
                        if (state.lastRhythmicMidiNote != -1) {
                             DEBUG_VERBOSE(CAT_MIDI, "Rhythmic MIDI Note Off Sent (Before Trigger): %d", state.lastRhythmicMidiNote);
                             sendMidiNoteOff(state.lastRhythmicMidiNote, 0, MIDI_CHANNEL);
                             stopNote(0); // Stop internal synth voice
                             state.lastRhythmicMidiNote = -1;
                        }
                        
                        // --- Trigger Note On --- 
                        int baseMidiNote = getBaseMidiNote(state); // Get note from controller buttons
                        if (baseMidiNote != -1) {
                            baseMidiNote -= 24; // Apply octave drop
                            if (baseMidiNote < 0) baseMidiNote = 0;
                            
                            DEBUG_INFO(CAT_PLAYSTYLE, "Rhythmic %s Trigger (Note Index %d): %d", (triggerL ? "L" : "R"), i, baseMidiNote);
                            playNote(state, 0, baseMidiNote);
                            sendMidiNoteOn(baseMidiNote, MIDI_VELOCITY, MIDI_CHANNEL);
                            state.lastRhythmicMidiNote = baseMidiNote;
                        }
                    } // End if (triggerL || triggerR)
                    
                    // Mark note as processed for this cycle, even if L/R weren't held
                    state.notePlayedInCycle[i] = true;

                } // End if (nowMicros >= targetMicrosForNote)
            } // End if (!notePlayedInCycle)
        } // End for loop through pattern notes

        // --- Handle Note Off if triggers released mid-cycle --- 
        if (!state.boogieLActive && !state.boogieRActive && state.lastRhythmicMidiNote != -1) {
             DEBUG_VERBOSE(CAT_MIDI, "Rhythmic MIDI Note Off Sent (Triggers Released): %d", state.lastRhythmicMidiNote);
             sendMidiNoteOff(state.lastRhythmicMidiNote, 0, MIDI_CHANNEL);
             stopNote(0);
             state.lastRhythmicMidiNote = -1;
        }

    } // End if (rhythmicModeEnabled && (midiSyncEnabled || tempoEstablished))
    
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
        
        // --- Mode Logic --- 
        // Check modes directly, Tempo availability handled within modes

        // Always update L/R active states
        state.boogieLActive = state.held[BTN_L];
        state.boogieRActive = state.held[BTN_R];
        
        if (state.boogieModeEnabled) {
            // If Boogie Mode is selected...
            if (state.tempoEstablished) { 
                 // Tempo is established (locked), run the rhythmic Boogie timing logic
                 handleBoogieTiming(state); 
            } else {
                 // Tempo not established (pre-Start, during sampling, or sampling failed)
                 // --> Fall back to acting like Monophonic mode <--
                 DEBUG_VERBOSE(CAT_PLAYSTYLE, "Boogie Mode Active, Tempo Not Established -> Running Monophonic");
                 handleMonophonic(state); // Call standard monophonic handler
            }
        } else if (state.rhythmicModeEnabled) {
            // Rhythmic mode timing logic is handled entirely by the high-resolution block earlier in the loop.
            // This block runs if rhythmicModeEnabled is true AND (midiSyncEnabled OR tempoEstablished) is true.
            // We don't need to call anything specific here, just ensure standard styles don't run.
            ; // Explicitly do nothing, handled above
        } else {
            // Neither Boogie nor Rhythmic mode active: Run Standard Playstyles
            runStandardPlaystyles(state);
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

    // --- Update Previous State for Next Loop ---
    state.prevMidiSyncEnabled = state.midiSyncEnabled; // Added for Boogie V12.5

    // --- MIDI Clock Timeout --- 
    // If clock is enabled but we haven't received a tick in a while, disable sync
    if (state.midiSyncEnabled && (millis() - state.lastMidiClockTime > MIDI_CLOCK_TIMEOUT_MS)) {
        DEBUG_WARNING(CAT_MIDI, "MIDI Clock Timeout! Disabling sync.");
        state.midiSyncEnabled = false; 
        // --- Keep Established Tempo --- 
        // state.tempoEstablished = false; // Keep true if already established
        // state.usPerMidiTick = 0.0f; // Keep the locked value
        state.isSamplingTempo = false; // Ensure sampling stops
        // Optionally stop notes here too if desired on timeout
        if (state.boogieModeEnabled && state.boogieCurrentMidiNote != -1) {
             DEBUG_INFO(CAT_MIDI, "Stopping Boogie note due to Clock Timeout");
             sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL);
             stopNote(0);
             state.boogieCurrentMidiNote = -1;
             state.boogieTriggerButton = -1; 
        }
    }

    // No delay in the main loop - let the audio system run as fast as possible
    yield();  // Allow other tasks to run if needed
}

// --- Helper function to run standard playstyles ---
void runStandardPlaystyles(SynthState& state) {
    // Update L/R active states (for potential use in standard mono/chord)
    state.boogieLActive = state.held[BTN_L];
    state.boogieRActive = state.held[BTN_R];
    
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
}

// --- MIDI Clock Handling --- 
void handleClock() {
    unsigned long nowMicros = micros();
    DEBUG_VERBOSE(CAT_MIDI, "handleClock() Entered. Sampling: %s", state.isSamplingTempo ? "Yes" : "No");

    // Previous lastTickTimeMicros for delta calculation
    unsigned long previousTickTimeMicros = state.lastTickTimeMicros;

    // Always update these for timeout checks etc.
    state.lastTickTimeMicros = nowMicros; 
    state.midiSyncEnabled = true; 
    state.lastMidiClockTime = millis();

    // --- Sample-and-Hold Logic --- 
    if (state.isSamplingTempo) {
        // --- Sampling Phase --- 
        if (previousTickTimeMicros > state.beatStartTimeMicros) { // Ensure we have at least one previous tick time after Start
             unsigned long deltaMicros = nowMicros - previousTickTimeMicros; // Use the previous tick time
             DEBUG_VERBOSE(CAT_MIDI, "  Sampling: deltaMicros = %lu", deltaMicros);

             if (deltaMicros > 0 && deltaMicros < 2000000) { // Basic validation
                 // Accumulate interval data 
                 state.samplingIntervalSum += (double)deltaMicros; // Add to sum
                 state.sampleTickCount++;
                 DEBUG_VERBOSE(CAT_MIDI, "  Sampling: sampleTickCount = %d/%d", state.sampleTickCount, NUM_SAMPLES_FOR_LOCK);

                 // Check if sampling is complete
                 if (state.sampleTickCount >= NUM_SAMPLES_FOR_LOCK) {
                     // Calculate the final average from the accumulated sum
                     float averageInterval = 0.0f;
                     if (state.sampleTickCount > 0) { // Ensure we don't divide by zero
                         averageInterval = (float)(state.samplingIntervalSum / (double)state.sampleTickCount);
                     }
                     
                     if (averageInterval > 0.0f) {
                        state.usPerMidiTick = averageInterval; // Set the final locked tempo
                        state.lockedUsPerMidiTick = averageInterval; // Store separately for clarity/future use if needed
                        state.tempoEstablished = true; // LOCK IT IN!
                        state.isSamplingTempo = false; // Stop sampling
                        float bpm = 60000000.0f / (24.0f * state.usPerMidiTick);
                        DEBUG_INFO(CAT_MIDI, "Tempo Sampling Complete. Locked Avg Interval: %.2f us (%.2f BPM) from %d samples", state.usPerMidiTick, bpm, state.sampleTickCount);
                     } else {
                         // Sampling failed (e.g., all deltas were invalid)
                         DEBUG_ERROR(CAT_MIDI, "Tempo Sampling Failed! Invalid average interval.");
                         state.tempoEstablished = false;
                         state.isSamplingTempo = false; // Stop trying
                     }
                 }
             } else {
                 // Invalid delta during sampling - ignore this tick for count/average?
                 DEBUG_WARNING(CAT_MIDI, "Invalid delta %lu us during Tempo Sampling - Tick ignored.", deltaMicros);
             }
        }
        // else: This is the very first tick after Start, don't process interval yet.

    } else if (state.tempoEstablished) {
        // --- Holding Phase --- 
        // Tempo is established and locked. Do nothing with incoming tick timing.
        // We just need the ticks for the timeout check (state.lastMidiClockTime updated above).
        // DEBUG_VERBOSE(CAT_MIDI, "Tick received (Holding Tempo)");
    } else {
        // --- Waiting Phase --- 
        // Should not happen if Start was received, but safety belt.
        // Waiting for MIDI Start.
    }
    
    // Removed previous averaging and phase correction logic here
}

void handleStart() {
    unsigned long nowMicros = micros();
    DEBUG_INFO(CAT_MIDI, "MIDI Start Received - Begin Tempo Sampling (%d ticks)", NUM_SAMPLES_FOR_LOCK);
    state.beatStartTimeMicros = nowMicros; 
    state.lastTickTimeMicros = nowMicros; 
    state.midiSyncEnabled = true;
    state.tempoEstablished = false; // IMPORTANT: Tempo is NOT established until sampling completes
    state.usPerMidiTick = 0.0f; // Clear current tempo
    state.lockedUsPerMidiTick = 0.0f; // Clear locked tempo
    
    // Reset Sampling State
    state.isSamplingTempo = true; 
    state.sampleTickCount = 0;
    state.samplingIntervalSum = 0.0; // Reset sum accumulator
    // Clear the interval buffer for fresh averaging during sampling
    state.tickBufferIndex = 0; // Using buffer index for averaging during sampling
    for(int i=0; i<MIDI_TICK_BUFFER_SIZE; ++i) state.tickIntervalBuffer[i] = 0.0f;
    state.tickBufferFilled = false; // Reset buffer filled flag

    // Reset Boogie state (from previous version)
    state.boogieTriggerButton = -1; 
    state.boogieCurrentMidiNote = -1;
    state.boogieCurrentSlotIndex = -1;
    state.boogieNoteStartTimeMicros = 0; // Initialize Start Time
    state.boogieNoteStopTimeMicros = 0;
    state.boogieInternalBeatStartTimeMicros = 0;

    state.lastMidiClockTime = millis();
}

void handleStop() {
    DEBUG_INFO(CAT_MIDI, "MIDI Stop Received");
    state.midiSyncEnabled = false; // Clock has stopped
    // --- Keep Established Tempo --- 
    // state.tempoEstablished = false; // Keep true if already established
    // state.usPerMidiTick = 0.0f; // Keep the locked value
    // state.lockedUsPerMidiTick = 0.0f; // Keep the locked value
    state.isSamplingTempo = false; // Ensure sampling stops if it was somehow active
    state.sampleTickCount = 0;
    // Reset tick buffer state (not strictly necessary but cleans up)
    state.tickBufferFilled = false;
    state.tickBufferIndex = 0;

    // Stop the audio note immediately on MIDI Stop
    if (state.boogieModeEnabled && state.boogieCurrentMidiNote != -1) {
         sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL);
         stopNote(0);
         state.boogieCurrentMidiNote = -1;
         state.boogieTriggerButton = -1; // Reset trigger too
         state.boogieCurrentSlotIndex = -1;
         state.boogieInternalBeatStartTimeMicros = 0;
         state.boogieNoteStartTimeMicros = 0; // Initialize Start Time
         state.boogieNoteStopTimeMicros = 0;
    }
     // Add similar logic for other playstyles if needed
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