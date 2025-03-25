// commands.cpp
// Implements functions for processing SNES controller button combinations,
// updating the synthesizer state (e.g., toggle portamento, cycle scales, change key).

#include "commands.h"
#include "chords.h"  // Include for NUM_PROFILES
#include <Arduino.h>

// Externally defined in synth.cpp
extern const int NUM_SCALES;

void checkCommands(SynthState& state) {
    // L + R + A to toggle portamento (buttons 10, 11, 9)
    if (state.orderHeld[10] >= 1 && state.orderHeld[11] >= 1 && state.pressed[9] == 1) {
        state.portamentoEnabled = !state.portamentoEnabled;
        Serial.print("Portamento Enabled: ");
        Serial.println(state.portamentoEnabled ? "True" : "False");
        delay(200);
    }
    // L + R + Y to cycle to the next scaleMode (buttons 10, 11, 6)
    if (state.orderHeld[10] >= 1 && state.orderHeld[11] >= 1 && state.pressed[6] == 1) {
        state.scaleMode = (state.scaleMode + 1) % NUM_SCALES;  // Cycle forward, loop back to 0 at the end
        Serial.print("Scale Mode Changed to: ");
        Serial.println(state.scaleMode);
        delay(200);
    }
    // L + R + X to increase keyOffset (buttons 10, 11, 8)
    if (state.orderHeld[10] >= 1 && state.orderHeld[11] >= 1 && state.pressed[8] == 1) {
        state.keyOffset = (state.keyOffset + 1) % 12;  // Increase key by 1 semitone, wrap around at 12
        Serial.print("Key Offset Changed to: ");
        Serial.println(state.keyOffset);
        delay(200);
    }
    // L + R + B to decrease keyOffset (buttons 10, 11, 7)
    if (state.orderHeld[10] >= 1 && state.orderHeld[11] >= 1 && state.pressed[7] == 1) {
        state.keyOffset = (state.keyOffset - 1 + 12) % 12;  // Decrease key by 1 semitone, wrap around at 0
        Serial.print("Key Offset Changed to: ");
        Serial.println(state.keyOffset);
        delay(200);
    }
    // L + R + Up to switch to ChordButton mode (buttons 10, 11, 2)
    if (state.orderHeld[10] >= 1 && state.orderHeld[11] >= 1 && state.pressed[2] == 1) {
        state.playStyle = CHORD_BUTTON;
        Serial.println("Switched to ChordButton mode");
        delay(200);
    }
    // L + R + Right to cycle chord profiles (buttons 10, 11, 3)
    if (state.orderHeld[10] >= 1 && state.orderHeld[11] >= 1 && state.pressed[3] == 1) {
        state.chordProfile = (state.chordProfile + 1) % NUM_PROFILES;  // Cycle forward, loop back to 0
        Serial.print("Chord Profile Changed to: ");
        Serial.println(state.chordProfile);
        delay(200);
    }
}