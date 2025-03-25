// chords.h
// Header file for chord-related functions, including chord definitions and
// generation of chord notes based on the active scale and profile.

#ifndef CHORDS_H
#define CHORDS_H

#include "synth_state.h"

// Number of chord profiles
#define NUM_PROFILES 2

// Function to get the MIDI notes for a chord based on the scale degree, scale, and profile
void getChordNotes(SynthState& state, int scaleDegree, int* chordNotes, int& numNotes);

#endif