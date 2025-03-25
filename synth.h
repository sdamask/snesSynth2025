// synth.h
// Header file for synthesizer functions, including scale management, MIDI note handling,
// and playstyle logic for the SNES synthesizer.

#ifndef SYNTH_H
#define SYNTH_H

#include "synth_state.h"

// Declare NUM_SCALES and SCALE_DEFINITIONS as extern variables
extern const int NUM_SCALES;
extern const int SCALE_DEFINITIONS[][10];
// Declare MIDI constants as extern variables
extern const int MIDI_CHANNEL;
extern const int MIDI_VELOCITY;

void initializeSynthState(SynthState& state);
void updateScale(SynthState& state);
void handleMonophonicPlayStyle(SynthState& state);
void sendMidiNoteOn(int note, int velocity, int channel);
void sendMidiNoteOff(int note, int velocity, int channel);

#endif