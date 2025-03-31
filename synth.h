// synth.h
// Header file for synthesizer functions, including scale management, MIDI note handling,
// and playstyle logic for the SNES synthesizer.

#ifndef SYNTH_H
#define SYNTH_H

#include "synth_state.h"

// Constants related to scales
const int NUM_SCALES = 7;
const int NUM_SCALE_NOTES = 10; // Max notes in a scale definition (including -1 terminator)
extern const int SCALE_DEFINITIONS[NUM_SCALES][NUM_SCALE_NOTES];

// Function declarations
void initializeSynthState(SynthState& state);
void updateScale(SynthState& state);
// Remove playstyle handlers if they were declared here, they belong in playstyles.h
// void handleMonophonic(SynthState& state); 
// void handlePolyphonic(SynthState& state);
// void handleChordButton(SynthState& state);

#endif // SYNTH_H