// synth_state.h
// Defines the SynthState struct, which holds the dynamic state of the SNES synthesizer,
// including button states, scale settings, MIDI note data, and synthesizer parameters.

#ifndef SYNTH_STATE_H
#define SYNTH_STATE_H

#define PRESSED 0
#define UNPRESSED 1

enum PlayStyle {
    MONOPHONIC,
    POLYPHONIC,
    CHORD_BUTTON
};

struct SynthState {
    // SNES controller state
    short snesRegister = 32767;
    short snesRegOld = 32767;
    short landing = 0;
    int scaleOrder2[12] = { 5, 6, 4, 7, 2, 3, 1, 0, 9, 8, 10, 11 };  // Down, Left, Up, Right, Select, Start, Y, B, X, A, L, R
    int pressed[12] = { 0 };
    int released[12] = { 0 };
    int held[12] = { 0 };
    int prevHeld[12] = { 0 };
    int orderHeld[12] = { 0 };
    int unpressed[12] = { 0 };
    int codeBuffer[12] = { 0 };  // Last 12 button presses

    // Polyphony and overflow (for future note playback)
    int nowPlaying[4] = { -1, -1, -1, -1 };  // Up to 4 notes at a time
    int waveformOpen[4] = { 1, 1, 1, 1 };    // Tracks available waveform slots
    int waveformOverflow[4] = { -1, -1, -1, -1 };  // Overflow buffer for extra notes
    int overflowCnt = 0;  // Number of notes in overflow

    // Synthesizer state
    PlayStyle playStyle = MONOPHONIC;  // Default to monophonic playStyle
    int scaleMode = 0;  // Diatonic scale/musical mode (0 = Major, 1 = Pentatonic, etc.)
    int scaleHolder[16] = { 0 };  // Computed scale notes
    int baseNote;  // Base MIDI note for the scale (e.g., 60 = C3), initialized in synth.cpp
    int keyOffset;  // Chromatic key transposition (0 = C, 1 = C#, etc.), initialized in synth.cpp
    int arpeggioOffset = 0;  // Offset for arpeggio octave swings (e.g., 0, 12, 24)
    int currentButton = -1;  // Button currently playing a note (for monophonic)
    int currentMidiNote = -1;  // MIDI note currently playing (for monophonic)
    int currentChordNotes[4] = { -1, -1, -1, -1 };  // MIDI notes of the current chord (for ChordButton)
    float currentChordFrequencies[4] = { 0.0, 0.0, 0.0, 0.0 };  // Frequencies of the current chord notes (for portamento)
    int chordProfile = 0;  // Current chord profile index (for ChordButton)
    int pitchBend = 0;  // Octave offset in semitones (+12 per octave up, -12 per octave down)
    bool portamentoEnabled = false;  // Toggle between immediate and portamento pitch changes
    float currentFrequency = 0.0;  // Current frequency for portamento (for monophonic)
    float degToRad = 3.14 / 180;  // For portamento sine curve (from your original code)
};

#endif