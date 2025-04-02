#ifndef SYNTH_STATE_H
#define SYNTH_STATE_H

#include <Arduino.h>
#include <stdint.h>

#define MAX_NOTE_BUTTONS 10
#define LAST_PRESS_BUFFER_SIZE 8 // Remember last 8 presses
#define MIDI_TICK_BUFFER_SIZE 8 // KEEP for initial averaging window if needed?
                                   // Let's reuse buffer but define sample count separately
#define NUM_SAMPLES_FOR_LOCK 24 // Number of ticks to sample before locking tempo

// Mapping Profiles
#define PROFILE_SCALE 0
#define PROFILE_THUNDERSTRUCK 1

// Play styles
enum PlayStyle {
    MONOPHONIC,
    POLYPHONIC,
    CHORD_BUTTON
};

struct SynthState {
    // Controller state
    short snesRegister = 32767;
    short snesRegOld = 32767;
    short landing = 0;
    
    // Button tracking
    bool held[12] = {0};
    bool prevHeld[12] = {0};
    uint8_t pressed[12] = {0};
    uint8_t released[12] = {0};
    int lastPressedBuffer[LAST_PRESS_BUFFER_SIZE];
    int lastPressedIndex = 0;
    
    // Synth state
    int baseNote = 60;  // Middle C
    int keyOffset = 0;  // No transposition
    int scaleMode = 0;  // Major scale
    int currentNote = -1;  // No note playing
    bool needsScaleUpdate = true;
    int scaleHolder[12];  // Computed scale notes
    
    // Voice tracking
    bool voiceActive[4] = {0};  // Track which voices are active
    int voiceToNote[4] = {-1};  // Which note each voice is playing
    
    // Play style
    PlayStyle playStyle = MONOPHONIC;
    int chordProfile = 0;  // Current chord type (major, minor, etc.)
    bool portamentoEnabled = false;
    int currentWaveform = 0; // 0: Sine, 1: Saw, 2: Square, 3: Triangle
    int vibratoRate = 1;     // Default to 5Hz (Index 1)
    int vibratoDepth = 2;    // Default to Medium (Index 2)
    int customProfileIndex = PROFILE_SCALE; // 0=Scale, 1=Thunderstruck, etc.

    // MIDI and pitch control
    int pitchBend = 0;
    int prevPitchBend = 0;
    int currentMidiNote = -1;
    int currentButton = -1;
    float currentFrequency = 0.0;
    
    // Chord state
    int currentChordNotes[4] = {-1, -1, -1, -1};
    float currentChordFrequencies[4] = {0.0, 0.0, 0.0, 0.0};
    bool waveformOpen[4] = {1, 1, 1, 1};  // 1 = open (no sound), 0 = closed (playing)
    
    // Additional state
    int arpeggioOffset = 0;
    char codeBuffer[32];  // For debug messages

    // Command handling flag
    bool commandJustExecuted = false; // Set to true by checkCommands if a combo was handled

    // --- Modes & MIDI Sync ---
    bool midiSyncEnabled = false;       // Is MIDI clock currently detected?
    bool tempoEstablished = false;      // Has a tempo ever been set by the MIDI clock?
    bool boogieModeEnabled = false;     // Is Boogie mode selected?
    bool rhythmicModeEnabled = false;   // Is Rhythmic mode selected?
    unsigned long lastTickTimeMicros = 0;
    float currentTempoBPM = 120.0f;     // Default tempo
    float ticksPerQuarterNote = 24.0f;

    // --- State for Boogie Mode (Tick-based) ---
    // uint32_t midiClockCount;      // Tick counter (0-23) for Boogie Mode
    // int boogieRTickValue;         // R Note trigger tick for Boogie Mode (swing)
    float boogieRTimingRatio = 0.5f; // Position of the 'R' note within the beat (0.5 = straight 8ths) - NO LONGER USED
    unsigned long beatStartTimeMicros = 0; // micros() timestamp when the current beat started - Still needed for V11 beat tracking
    // bool notePlayedInBeat[2] = {false, false}; // Track if L (0) and R (1) notes played this beat - NO LONGER USED in V10
    // bool boogieNoteIsPlaying = false; // Is a boogie note currently supposed to be sounding? V8/V9 - Replaced by boogieIsCurrentlyPlaying

    // State for V11 (Variable Duration 8th Notes)
    int boogieTriggerButton = -1;           // Which button (0-9) triggered the current sequence? -1 if inactive.
    int boogieCurrentSlotIndex = -1;        // Which 8th note slot is currently active? (0 or 1, -1 if inactive)
    unsigned long boogieNoteStartTimeMicros = 0; // Micros() time when the current note started (NEW)
    unsigned long boogieNoteStopTimeMicros = 0;  // Micros() time when the current note should stop
    int boogieCurrentMidiNote = -1;         // MIDI note number currently sounding, -1 if silent.
    float firstEighthNoteDurationRatio = 0.25f; // Default: Short first note (25% of slot)
    float secondEighthNoteDurationRatio = 0.90f;// Default: Long second note (90% of slot)

    // --- State for Rhythmic Mode (Micros()-based) ---
    float usPerMidiTick = 0.0f;
    unsigned long lastMidiClockTime = 0;
    unsigned long cycleStartTimeMicros; // micros() timestamp when the current rhythm cycle started
    int numNotesInPattern;                     
    static const int MAX_PATTERN_NOTES = 16;   
    float currentRhythmPatternTicks[MAX_PATTERN_NOTES]; 
    bool notePlayedInCycle[MAX_PATTERN_NOTES]; 
    float currentRhythmPatternLengthTicks;     
    // Keep L/R trigger active flags - USED BY BOTH MODES?
    bool boogieLActive; 
    bool boogieRActive; 
    // Need last note for rhythmic mode too!
    int lastRhythmicMidiNote; // Last note played in Rhythmic Mode

    // Tempo Averaging / Locking
    float tickIntervalBuffer[MIDI_TICK_BUFFER_SIZE] = {0.0f}; // Buffer for intervals
    int tickBufferIndex = 0; // Index for buffer (might not be needed if sampling logic changes)
    bool tickBufferFilled = false; // Might not be needed
    // int midiTickCounter = 0; // REMOVE - No longer used for phase correction

    // Sample and Hold State (NEW)
    bool isSamplingTempo = false; // True during initial N ticks after Start
    int sampleTickCount = 0; // Counts ticks collected during sampling phase
    float lockedUsPerMidiTick = 0.0f; // Stores the locked tempo value (used indirectly via usPerMidiTick)
    double samplingIntervalSum = 0.0; // Accumulates sum of intervals during sampling (use double for precision)

    // Added for Swing
    float swingAmount = 0.0f; // Swing amount (0.0 = even, 1.0 = full triplet feel 2/3 delay)

    // Added for Boogie V12.5
    bool prevMidiSyncEnabled = false;
    unsigned long boogieInternalBeatStartTimeMicros = 0;
};

#endif // SYNTH_STATE_H