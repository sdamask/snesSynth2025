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
    Serial.println("**********");
    Serial.print("playStyle: ");
    switch (state.playStyle) {
        case MONOPHONIC:
            Serial.println("Monophonic");
            break;
        case POLYPHONIC:
            Serial.println("Polyphonic");
            break;
        case CHORD_BUTTON:
            Serial.println("ChordButton");
            break;
        default:
            Serial.println("Unknown");
            break;
    }

    Serial.print("Key (keyOffset): ");
    switch (state.keyOffset) {
        case 0: Serial.println("C"); break;
        case 1: Serial.println("C#"); break;
        case 2: Serial.println("D"); break;
        case 3: Serial.println("D#"); break;
        case 4: Serial.println("E"); break;
        case 5: Serial.println("F"); break;
        case 6: Serial.println("F#"); break;
        case 7: Serial.println("G"); break;
        case 8: Serial.println("G#"); break;
        case 9: Serial.println("A"); break;
        case 10: Serial.println("A#"); break;
        case 11: Serial.println("B"); break;
        default: Serial.println("Unknown"); break;
    }

    Serial.print("Scale (scaleMode): ");
    Serial.println(state.scaleMode);

    Serial.print("Pitch Bend (semitones): ");
    Serial.println(state.pitchBend);

    Serial.print("Portamento Enabled: ");
    Serial.println(state.portamentoEnabled ? "True" : "False");

    // Print Vibrato Status
    const char* rateNames[] = {"Off", "5Hz", "10Hz"};
    const char* depthNames[] = {"Off", "Low", "Medium", "High"};
    Serial.print("Vibrato: ");
    Serial.print(rateNames[state.vibratoRate]);
    Serial.print(" / ");
    Serial.println(depthNames[state.vibratoDepth]);

    // REMOVED Note-Specific Info
    /*
    // ... note info ...
    */

    // REMOVED Button State Arrays
    /*
    Serial.print("Held:      "); 
    for (int i = 0; i < 12; i++) { 
        Serial.print(state.held[i]);
    }
    Serial.println();
    */

    // REMOVED Last Pressed Buffer Printout
    /*
    Serial.print("Last Pressed Buf (idx ");
    Serial.print(state.lastPressedIndex);
    Serial.print("): [ ");
    // ... loop ...
    Serial.println(" ]"); 
    */

    Serial.println("**********");
}