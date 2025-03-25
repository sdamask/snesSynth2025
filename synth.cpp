// synth.cpp
// Implements synthesizer functions for the SNES synthesizer, including scale definitions,
// MIDI note generation, and playstyle handling.

#include "synth.h"
#include "audio.h"
#include "midi_utils.h"
#include <Arduino.h>  // For Serial, delay, sin, etc.

// Global scale definitions
const int SCALE_DEFINITIONS[4][10] = {
    { 0, 2, 4, 5, 7, 9, 11, -1 },  // Major (C, D, E, F, G, A, B)
    { 0, 2, 4, 7, 9, -1 },         // Major Pentatonic (C, D, E, G, A)
    { 0, 2, 3, 5, 7, 9, 10, -1 },  // Dorian (C, D, Eb, F, G, A, Bb)
    { 0, 1, 3, 5, 7, 8, 10, -1 }   // Phrygian (C, Db, Eb, F, G, Ab, Bb)
};
const int NUM_SCALES = sizeof(SCALE_DEFINITIONS) / sizeof(SCALE_DEFINITIONS[0]);

// Default values for baseNote and keyOffset
const int DEFAULT_BASE_NOTE = 60;  // C3
const int DEFAULT_KEY_OFFSET = 0;  // No transposition (C)

// MIDI channel (1-16, default to channel 1)
const int MIDI_CHANNEL = 1;
// Default velocity (0-127, default to 100)
const int MIDI_VELOCITY = 100;

// Initialize SynthState with default values
void initializeSynthState(SynthState& state) {
    state.baseNote = DEFAULT_BASE_NOTE;
    state.keyOffset = DEFAULT_KEY_OFFSET;
}

void sendMidiNoteOn(int note, int velocity, int channel) {
    Serial.print("Sending MIDI Note On: ");
    Serial.print(note);
    Serial.print(", Velocity: ");
    Serial.println(velocity);
    usbMIDI.sendNoteOn(note, velocity, channel);
}

void sendMidiNoteOff(int note, int velocity, int channel) {
    Serial.print("Sending MIDI Note Off: ");
    Serial.print(note);
    Serial.print(", Velocity: ");
    Serial.println(velocity);
    usbMIDI.sendNoteOff(note, velocity, channel);
}

void updateScale(SynthState& state) {
    // Select the scale based on scaleMode (default to Major if out of range)
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

    // Generate MIDI notes dynamically
    for (int i = 0; i < 10; i++) {
        int degree = i % scaleLength;  // Repeat the scale after scaleLength notes
        int octaveOffset = (i / scaleLength) * 12;  // Add 12 semitones for each octave
        state.scaleHolder[i] = state.baseNote + intervals[degree] + octaveOffset + state.keyOffset + state.arpeggioOffset;
    }
}
