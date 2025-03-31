// controller.cpp
// Implements functions to read and process SNES controller button states,
// updating the SynthState with pressed, released, and held button information.

#include "controller.h"
#include "debug.h"
#include <Arduino.h>

// Button order mapping (scaleorder2)
// Maps raw SNES bits to desired order: down, left, up, right, select, start, y, b, x, a
// CORRECTED: Direct 1-to-1 mapping. The musical order is handled by playstyles.
const int buttonOrder[12] = {
    0,  // B (bit 0) -> BTN_B (0)
    1,  // Y (bit 1) -> BTN_Y (1)
    2,  // Select (bit 2) -> BTN_SELECT (2)
    3,  // Start (bit 3) -> BTN_START (3)
    4,  // Up (bit 4) -> BTN_UP (4)
    5,  // Down (bit 5) -> BTN_DOWN (5)
    6,  // Left (bit 6) -> BTN_LEFT (6)
    7,  // Right (bit 7) -> BTN_RIGHT (7)
    8,  // A (bit 8) -> BTN_A (8)
    9,  // X (bit 9) -> BTN_X (9)
    10, // L (bit 10) -> BTN_L (10)
    11  // R (bit 11) -> BTN_R (11)
};

void setupController() {
    // Set up pins for SNES controller
    pinMode(SNES_CLOCK, OUTPUT);
    pinMode(SNES_LATCH, OUTPUT);
    pinMode(SNES_DATA, INPUT);
    
    // Initialize pins to default state
    digitalWrite(SNES_CLOCK, HIGH);
    digitalWrite(SNES_LATCH, LOW);
}

void buttonState(SynthState& state) {
    // Save previous state
    state.snesRegOld = state.snesRegister;
    
    // Save previous held state
    for (int i = 0; i < 12; i++) {
        state.prevHeld[i] = state.held[i];
        state.pressed[i] = 0;
        state.released[i] = 0;
        state.held[i] = 0;
    }
    
    // Latch controller state
    digitalWrite(SNES_LATCH, HIGH);
    delayMicroseconds(12);
    digitalWrite(SNES_LATCH, LOW);
    
    // Read controller data
    state.snesRegister = 0;
    for (int i = 0; i < 16; i++) {
        state.snesRegister |= digitalRead(SNES_DATA) << i;
        digitalWrite(SNES_CLOCK, LOW);
        delayMicroseconds(6);
        digitalWrite(SNES_CLOCK, HIGH);
        delayMicroseconds(6);
    }
    
    // Process button states using the button order mapping
    for (int rawBit = 0; rawBit < 12; rawBit++) {
        int mappedIndex = buttonOrder[rawBit]; // This mapping is now direct
        bool buttonPressed = !(state.snesRegister & (1 << rawBit));
        
        // Update button state
        state.held[mappedIndex] = buttonPressed;
        state.pressed[mappedIndex] = buttonPressed && !state.prevHeld[mappedIndex];
        state.released[mappedIndex] = !buttonPressed && state.prevHeld[mappedIndex];

        // If any button (0-11) was just pressed, add it to the buffer
        if (state.pressed[mappedIndex]) {
            state.lastPressedBuffer[state.lastPressedIndex] = mappedIndex;
            state.lastPressedIndex = (state.lastPressedIndex + 1) % LAST_PRESS_BUFFER_SIZE;
            // Optional: Add debug print here if needed
            // DEBUG_DEBUG(CAT_BUTTON, "Added button %d (%s) to press buffer", mappedIndex, buttonNames[mappedIndex]);
        }
    }
}