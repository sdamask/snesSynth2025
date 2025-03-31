#include "midi.h"
#include <MIDI.h> // Assuming standard MIDI library is used

// Define MIDI Constants
const int MIDI_CHANNEL = 1;
const int MIDI_VELOCITY = 100; // A common default velocity

void sendMidiNoteOn(int note, int velocity, int channel) {
    // Assuming 'usbMIDI' is the instance name from the Teensy USB MIDI setup
    usbMIDI.sendNoteOn(note, velocity, channel);
    usbMIDI.send_now(); // Ensure data is sent immediately
}

void sendMidiNoteOff(int note, int velocity, int channel) {
    usbMIDI.sendNoteOff(note, velocity, channel);
    usbMIDI.send_now();
}

// Include other MIDI related functions if any
