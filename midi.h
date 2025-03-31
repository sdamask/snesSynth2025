#ifndef MIDI_H
#define MIDI_H

#include <Arduino.h> // Include Arduino for basic types if needed

// MIDI Constants
extern const int MIDI_CHANNEL;
extern const int MIDI_VELOCITY;

// MIDI functions
void sendMidiNoteOn(int note, int velocity, int channel);
void sendMidiNoteOff(int note, int velocity, int channel);

#endif // MIDI_H
