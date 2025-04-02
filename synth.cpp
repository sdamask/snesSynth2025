// synth.cpp
// Implements synthesizer functions for the SNES synthesizer, including scale definitions,
// MIDI note generation, and playstyle handling.

#include "synth.h"
#include "audio.h"
#include "debug.h"
#include <Arduino.h>

// Global scale definitions
// Match dimensions from synth.h: NUM_SCALES x NUM_SCALE_NOTES (7x10)
const int SCALE_DEFINITIONS[NUM_SCALES][NUM_SCALE_NOTES] = {
    { 0, 2, 4, 5, 7, 9, 11, -1 },     // Major (8 elements, fits in 10)
    { 0, 2, 3, 5, 7, 8, 10, -1 },     // Natural Minor (8 elements)
    { 0, 2, 3, 5, 7, 8, 11, -1 },     // Harmonic Minor (8 elements)
    { 0, 2, 3, 5, 7, 9, 11, -1 },     // Melodic Minor (8 elements)
    { 0, 2, 4, 6, 7, 9, 11, -1 },     // Lydian (8 elements)
    { 0, 2, 4, 5, 7, 9, 10, -1 },     // Mixolydian (8 elements)
    { 0, 2, 3, 5, 7, 8, 10, -1 }      // Dorian (8 elements)
    // C++ initializes remaining elements in each row to 0 automatically, which is fine
    // as the -1 terminator is encountered first.
};

// Default values for baseNote and keyOffset
const int DEFAULT_BASE_NOTE = 60;  // Middle C
const int DEFAULT_KEY_OFFSET = 0;  // No transposition

// MIDI channel (1-16, default to channel 1)
const int MIDI_CHANNEL = 1;
// Default velocity (0-127, default to 100)
const int MIDI_VELOCITY = 100;

// Initialize SynthState with default values
void initializeSynthState(SynthState& state) {
    state.baseNote = DEFAULT_BASE_NOTE;  // Middle C
    state.keyOffset = DEFAULT_KEY_OFFSET;  // No transposition
    state.playStyle = MONOPHONIC;
    state.needsScaleUpdate = true;
    state.portamentoEnabled = false;
    state.currentWaveform = 0; // Default to Sine
    state.vibratoRate = 1;     // Default to 5Hz (Index 1)
    state.vibratoDepth = 2;    // Default to Medium (Index 2)
    state.customProfileIndex = PROFILE_SCALE; // Default back to standard scale profile
    
    // Initialize arrays
    for (int i = 0; i < 12; i++) {
        state.held[i] = 0;
        state.prevHeld[i] = 0;
        state.pressed[i] = 0;
        state.released[i] = 0;
    }
    // Initialize last pressed buffer (e.g., with -1)
    for (int i = 0; i < LAST_PRESS_BUFFER_SIZE; ++i) {
        state.lastPressedBuffer[i] = -1;
    }
    state.lastPressedIndex = 0;
    
    // Initialize MIDI sync and rhythmic mode
    state.midiSyncEnabled = false;
    state.boogieModeEnabled = false; // Start with Boogie OFF by default
    state.rhythmicModeEnabled = false; // Start with Rhythmic OFF by default
    
    // Boogie State Init
    state.boogieRTimingRatio = 0.5f; // Ensure this is initialized
    state.beatStartTimeMicros = 0;   // Initialize new variable
    // state.notePlayedInBeat[0] = false; // Initialize new variable - REMOVED for V10
    // state.notePlayedInBeat[1] = false; // Initialize new variable - REMOVED for V10
    // state.lastBoogieMidiNote = -1;     // Ensure this is initialized - REMOVED for V11
    // Initialize V11 state
    state.boogieTriggerButton = -1;
    state.boogieNoteStopTimeMicros = 0;
    state.boogieCurrentMidiNote = -1;
    state.boogieCurrentSlotIndex = -1;
    // state.boogieIsCurrentlyPlaying = false; // REMOVED for V11
    
    // Rhythmic State Init (keep existing init)
    state.lastRhythmicMidiNote = -1; // Initialize new variable
    state.boogieLActive = false;
    state.boogieRActive = false;

    // Initialize Micros()-based Timing State
    state.lastTickTimeMicros = 0;
    state.usPerMidiTick = 20833.33f; // Default: 120 BPM -> (60 * 1e6 / 120 BPM / 24 PPQN)
    state.cycleStartTimeMicros = 0;
    
    // Initialize Default Rhythm Pattern (5 notes in 48 ticks)
    state.numNotesInPattern = 5;
    state.currentRhythmPatternLengthTicks = 48.0f;
    float ticksPerNote = state.currentRhythmPatternLengthTicks / state.numNotesInPattern; // 9.6
    for (int i = 0; i < state.numNotesInPattern; ++i) {
        state.currentRhythmPatternTicks[i] = i * ticksPerNote;
        state.notePlayedInCycle[i] = false;
    }
    // Clear remaining slots in pattern arrays
    for (int i = state.numNotesInPattern; i < state.MAX_PATTERN_NOTES; ++i) {
        state.currentRhythmPatternTicks[i] = 0.0f;
        state.notePlayedInCycle[i] = false;
    }

    // Initialize debug system
    
    updateScale(state);
}

void updateScale(SynthState& state) {
    if (!state.needsScaleUpdate) return;
    
    // Get current scale intervals
    const int* intervals = SCALE_DEFINITIONS[state.scaleMode];
    
    // Calculate scale length
    int scaleLength = 0;
    while (intervals[scaleLength] != -1 && scaleLength < 10) {
        scaleLength++;
    }
    
    // Fill scale holder with actual MIDI notes
    for (int i = 0; i < 12; i++) {
        int octave = i / scaleLength;
        int degree = i % scaleLength;
        state.scaleHolder[i] = state.baseNote + intervals[degree] + (octave * 12) + state.keyOffset;
    }
    
    state.needsScaleUpdate = false;
}

