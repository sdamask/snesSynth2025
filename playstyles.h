// playstyles.h
// Header file for playstyle functions, handling different modes of note playback
// (Monophonic, ChordButton, Polyphonic) for the SNES synthesizer.

#ifndef PLAYSTYLES_H
#define PLAYSTYLES_H

#include "synth_state.h"
#include "midi.h"

// Constants for MIDI
extern const int MIDI_CHANNEL;
extern const int MIDI_VELOCITY;

// Renamed functions to match calls in main.ino
void handleMonophonic(SynthState& state);
void handleChordButton(SynthState& state);
void handlePolyphonic(SynthState& state); // Add declaration for polyphonic

#endif