// playstyles.h
// Header file for playstyle functions, handling different modes of note playback
// (Monophonic, ChordButton, Polyphonic) for the SNES synthesizer.

#ifndef PLAYSTYLES_H
#define PLAYSTYLES_H

#include "synth_state.h"

void handleMonophonicPlayStyle(SynthState& state);
void handleChordButtonPlayStyle(SynthState& state);

#endif