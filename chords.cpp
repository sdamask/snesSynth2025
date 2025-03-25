// chords.cpp
// Implements functions for generating chord notes based on the active scale,
// scale degree, and chord profile for the SNES synthesizer.

#include "chords.h"

// Externally defined in synth.cpp
extern const int SCALE_DEFINITIONS[][10];
extern const int NUM_SCALES;

// Chord definitions: [profile][scale degree][notes]
// Each entry specifies scale degrees relative to the root (e.g., {1, 3, 5, 8} for a basic chord)
// Scale degree 1 corresponds to the root note of the chord (not the scale)
const int CHORD_DEFINITIONS[NUM_PROFILES][10][4] = {
    // Profile 0: Basic 1-3-5-8 (diatonic within the scale)
    {
        {1, 3, 5, 8},  // Degree 1: I chord
        {1, 3, 5, 8},  // Degree 2: ii chord
        {1, 3, 5, 8},  // Degree 3: iii chord
        {1, 3, 5, 8},  // Degree 4: IV chord
        {1, 3, 5, 8},  // Degree 5: V chord
        {1, 3, 5, 8},  // Degree 6: vi chord
        {1, 3, 5, 8},  // Degree 7: vii chord
        {1, 3, 5, 8},  // Degree 8: I chord (octave)
        {1, 3, 5, 8},  // Degree 9: ii chord (octave)
        {1, 3, 5, 8}   // Degree 10: iii chord (octave)
    },
    // Profile 1: Custom (example in A Dorian: ii as B/G)
    {
        {1, 3, 5, 8},   // Degree 1: I chord (A minor: A-C-E-A)
        {-2, 1, 3, 5},  // Degree 2: ii as G/B (G major with B in bass: B-G-B-D)
        {1, 3, 5, 8},   // Degree 3: iii chord
        {1, 3, 5, 8},   // Degree 4: IV chord
        {1, 3, 5, 8},   // Degree 5: V chord
        {1, 3, 5, 8},   // Degree 6: vi chord
        {1, 3, 5, 8},   // Degree 7: vii chord
        {1, 3, 5, 8},   // Degree 8: I chord (octave)
        {1, 3, 5, 8},   // Degree 9: ii chord (octave)
        {1, 3, 5, 8}    // Degree 10: iii chord (octave)
    }
};

void getChordNotes(SynthState& state, int scaleDegree, int* chordNotes, int& numNotes) {
    // Select the active scale
    int scaleIndex = (state.scaleMode >= 0 && state.scaleMode < NUM_SCALES) ? state.scaleMode : 0;
    const int* intervals = SCALE_DEFINITIONS[scaleIndex];

    // Calculate the scale length by finding the -1 terminator
    int scaleLength = 0;
    for (int i = 0; i < 10; i++) {
        if (intervals[i] == -1) {
            scaleLength = i;
            break;
        }
    }

    // Get the root MIDI note for the scale degree (1-based)
    int rootIndex = (scaleDegree - 1) % scaleLength;  // 0-based index in the scale
    int rootOctaveOffset = ((scaleDegree - 1) / scaleLength) * 12;  // Octave offset
    int rootMidiNote = state.baseNote + intervals[rootIndex] + rootOctaveOffset + state.keyOffset + state.arpeggioOffset;

    // Get the chord definition for the current profile and scale degree
    int profileIndex = state.chordProfile;
    const int* chordDef = CHORD_DEFINITIONS[profileIndex][scaleDegree - 1];  // 0-based scale degree

    // Calculate the MIDI notes for the chord
    numNotes = 0;
    for (int i = 0; i < 4; i++) {
        int degree = chordDef[i];
        if (degree == 0) break;  // End of chord definition (e.g., if fewer than 4 notes)

        // Calculate the scale degree relative to the current scale degree (1-based)
        // For example, if scaleDegree is 2 (D in C major), degree 1 means D, degree 3 means F, etc.
        int adjustedDegree = (scaleDegree - 1) + (degree - 1);  // 0-based relative degree
        int degreeOctaveOffset = (adjustedDegree / scaleLength) * 12;  // Octave offset
        int degreeIndex = adjustedDegree % scaleLength;  // Wrap around within the scale
        if (degreeIndex < 0) {
            degreeIndex += scaleLength;  // Handle negative degrees (e.g., -2)
            degreeOctaveOffset -= 12;  // Adjust octave for negative degrees
        }

        // Calculate the MIDI note
        int midiNote = state.baseNote + intervals[degreeIndex] + degreeOctaveOffset + state.keyOffset + state.arpeggioOffset;
        chordNotes[i] = midiNote;
        numNotes++;
    }
}