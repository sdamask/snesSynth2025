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

    Serial.print("Current MIDI Note: ");
    if (state.currentMidiNote == -1) {
        Serial.println("None");
    } else {
        // Display MIDI note number, note name, and octave
        Serial.print(state.currentMidiNote);
        Serial.print(" (");
        const char* noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        int noteIndex = state.currentMidiNote % 12;
        int octave = state.currentMidiNote / 12 - 2;  // MIDI note 0 is C-1, MIDI note 12 is C0, etc.
        Serial.print(noteNames[noteIndex]);
        Serial.print(octave);
        Serial.print(")");
    }
    Serial.println();

    // Calculate and display the scale degree
    Serial.print("Scale Degree: ");
    if (state.currentMidiNote == -1) {
        Serial.println("None");
    } else {
        // Define the desired button order for scale degrees
        const int scaleDegreeOrder[10] = { 5, 6, 4, 7, 2, 3, 1, 0, 9, 8 };  // Down, Left, Up, Right, Select, Start, Y, B, X, A
        // Find the root MIDI note (Down button, index 5)
        int rootMidiNote = state.scaleHolder[5];  // Base MIDI note for the root (without pitchBend)
        int adjustedRootMidiNote = rootMidiNote + state.pitchBend;
        // Find the current button's index in scaleDegreeOrder
        int currentButtonIndex = -1;
        for (int i = 0; i < 10; i++) {
            if (state.currentButton == scaleDegreeOrder[i]) {
                currentButtonIndex = i;
                break;
            }
        }
        if (currentButtonIndex == -1) {
            Serial.println("Not in scale");
        } else {
            // Use the button index directly as the scale degree (1-based)
            int scaleDegree = currentButtonIndex + 1;  // Map to 1-10
            Serial.print(scaleDegree);
            // Add octave information
            int noteDifference = state.currentMidiNote - adjustedRootMidiNote;
            int octaveOffset = noteDifference / 12;
            if (octaveOffset != 0) {
                Serial.print(" (octave ");
                Serial.print(octaveOffset);
                Serial.print(")");
            }
            Serial.println();
        }
    }

    // Serial.print("Pressed:   "); // REMOVED
    // for (int i = 0; i < MAX_NOTE_BUTTONS; i++) {
    //     Serial.print(state.pressed[i]);
    // }
    // Serial.println();

    // Serial.print("Released:  "); // REMOVED
    // for (int i = 0; i < MAX_NOTE_BUTTONS; i++) {
    //     Serial.print(state.released[i]);
    // }
    // Serial.println();

    Serial.print("Held:      "); // Add Held status line
    for (int i = 0; i < 12; i++) { // Loop 0-11 to include L/R
        Serial.print(state.held[i]);
    }
    Serial.println();
    
    // Serial.print("OrderHeld: ");
    // for (int i = 0; i < 12; i++) {
    //     Serial.print(state.orderHeld[i]);
    //     if (i < 11) Serial.print(" ");
    // }
    // Serial.println();
    
    Serial.print("Last Pressed Buf (idx ");
    Serial.print(state.lastPressedIndex);
    Serial.print("): [ ");
    // Print buffer contents from newest to oldest
    for (int i = 0; i < LAST_PRESS_BUFFER_SIZE; i++) { 
        // Calculate index to read, starting from last written and going back
        int readIndex = (state.lastPressedIndex + LAST_PRESS_BUFFER_SIZE - 1 - i) % LAST_PRESS_BUFFER_SIZE;
        int btnIndex = state.lastPressedBuffer[readIndex];
        if (btnIndex >= 0 && btnIndex < 12) { // Check if index is valid
             Serial.print(buttonNames[btnIndex]); // Print name
        } else {
             Serial.print("?"); // Print placeholder for invalid/unset
        }
        
        if (i < LAST_PRESS_BUFFER_SIZE - 1) {
            Serial.print(", ");
        }
    }
    Serial.println(" ]"); // Add closing bracket
    Serial.println("**********");
}