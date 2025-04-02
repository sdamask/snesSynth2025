// utils.cpp
// Implements utility functions for the SNES synthesizer, including printing
// the current state (e.g., playstyle, key, scale, MIDI note) to the Serial monitor.

#include "utils.h"
#include "button_defs.h" // Include for BTN_ defines
#include <Arduino.h>

// Array to map BTN_ index to readable names (for printing)
const char* buttonNames[12] = {
    "B", "Y", "Sel", "St", "Up", "Down", "Left", "Right", "A", "X", "L", "R"
};

void printStatus(SynthState& state) {
    // Print Mode
    Serial.print("MODE:");
    switch (state.playStyle) {
        case MONOPHONIC:    Serial.print("Mono"); break;
        case POLYPHONIC:    Serial.print("Poly"); break;
        case CHORD_BUTTON:  Serial.print("Chord"); break;
        default:            Serial.print("?"); break;
    }
    if (state.midiSyncEnabled) {
        if (state.boogieModeEnabled) Serial.print("(Boogie)");
        else if (state.rhythmicModeEnabled) Serial.print("(Rhythm)");
    }

    // Print Profile
    Serial.print(" | PROFILE:");
    Serial.print(state.customProfileIndex == PROFILE_SCALE ? "Scale" : "Thunder");

    // Print Key
    Serial.print(" | KEY:");
    const char* keyNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    if (state.keyOffset >= 0 && state.keyOffset < 12) {
        Serial.print(keyNames[state.keyOffset]);
    } else {
        Serial.print("?");
    }

    // Print Scale Mode
    Serial.print(" | SCALE:");
    Serial.print(state.scaleMode);

    // Print Portamento
    Serial.print(" | PORTA:");
    Serial.print(state.portamentoEnabled ? "On" : "Off");

    // Print Vibrato
    const char* rateNames[] = {"Off", "5Hz", "10Hz"};
    const char* depthNames[] = {"Off", "L", "M", "H"}; // Shortened depth names
    Serial.print(" | VIB:");
    if (state.vibratoRate >= 0 && state.vibratoRate < 3) Serial.print(rateNames[state.vibratoRate]); else Serial.print("?");
    Serial.print("/");
    if (state.vibratoDepth >= 0 && state.vibratoDepth < 4) Serial.print(depthNames[state.vibratoDepth]); else Serial.print("?");

    // Print Waveform (Optional - add if desired)
    // const char* waveformNames[] = {"Sin", "Saw", "Sqr", "Tri"};
    // Serial.print(" | WAVE:");
    // if (state.currentWaveform >= 0 && state.currentWaveform < 4) Serial.print(waveformNames[state.currentWaveform]); else Serial.print("?");

    Serial.println(); // Finish the line
}