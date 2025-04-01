#ifndef SYNTH_STATE_H
#define SYNTH_STATE_H

#include <Arduino.h>
#include <stdint.h>

#define MAX_NOTE_BUTTONS 10
#define LAST_PRESS_BUFFER_SIZE 8 // Remember last 8 presses

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

    // MIDI Sync and Boogie Mode
    bool midiSyncEnabled;
    uint32_t midiClockCount;
    uint32_t lastQuarterNoteTime;
    uint32_t midiTempo;  // in BPM
    bool boogieModeEnabled;
    bool boogieLActive; // Set by playstyles based on held[BTN_L]
    bool boogieRActive; // Set by playstyles based on held[BTN_R]
    uint8_t currentBeat;  // 0-3 for 4/4 time
    int lastBoogieMidiNote; // Track the last note played by boogie mode
    int boogieRTickValue; // Tick for the R note (swing)

    // Constructor
};

#endif // SYNTH_STATE_H