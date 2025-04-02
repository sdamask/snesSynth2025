// audio.h
// Header file for audio-related functions, including setup and note playback
// for the SNES synthesizer using the Teensy Audio Library.

#ifndef AUDIO_H
#define AUDIO_H

#include <Audio.h>
#include "synth_state.h"

// Forward declarations for audio components
extern AudioSynthWaveform waveform[4];
extern AudioSynthWaveformModulated waveformMod[4];
extern AudioEffectEnvelope envelope[4];
extern AudioSynthWaveform lfo[4];
extern AudioMixer4 mixer;  // Mixer to combine all voices
extern AudioOutputI2S i2s1;
extern AudioControlSGTL5000 sgtl5000_1;  // Audio shield
extern AudioConnection* patchCords[16];  // 4 voices to mixer (4), mixer to left (1), mixer to right (1), plus waveform to waveformMod (4) and waveformMod to envelope (4)

// Audio function declarations
void setupAudio();
void playNote(SynthState& state, int voice, int midiNote);
void stopNote(int voice);
void updateAudio(SynthState& state);

// MIDI Clock and Boogie Mode
// void processMidiTick(SynthState& state); // Removed - Logic moved to main loop/callbacks
int getBaseMidiNote(SynthState& state);

#endif