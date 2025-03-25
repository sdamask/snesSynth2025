// controller.cpp
// Implements functions to read and process SNES controller button states,
// updating the SynthState with pressed, released, and held button information.

#include "controller.h"
#include <Arduino.h>

void setupController() {
    pinMode(SNES_DATA, INPUT);
    pinMode(SNES_CLOCK, OUTPUT);
    pinMode(SNES_LATCH, OUTPUT);
    digitalWrite(SNES_CLOCK, LOW);
    digitalWrite(SNES_LATCH, LOW);
}

short readSnesController(SynthState& state) {
    short tempData = 32767;
    digitalWrite(SNES_LATCH, HIGH);
    digitalWrite(SNES_LATCH, LOW);

    if (digitalRead(SNES_DATA) == LOW)
        bitClear(tempData, 0);

    for (int i = 1; i <= 11; i++) {
        digitalWrite(SNES_CLOCK, HIGH);
        digitalWrite(SNES_CLOCK, LOW);
        if (digitalRead(SNES_DATA) == LOW)
            bitClear(tempData, i);
    }
    bitWrite(state.landing, 0, bitRead(tempData, state.scaleOrder2[0]));
    for (int i = 1; i <= 11; i++) {
        bitWrite(state.landing, i, bitRead(tempData, state.scaleOrder2[i]));
    }
    return state.landing;
}

void buttonState(SynthState& state) {
    state.snesRegOld = state.snesRegister;
    state.snesRegister = readSnesController(state);

    // Update button states
    for (int i = 0; i <= 11; i++) {
        if (bitRead(state.snesRegister, i) == PRESSED && bitRead(state.snesRegOld, i) == UNPRESSED) {
            state.pressed[i] = 1;
            // Update codeBuffer (last 12 button presses)
            for (int j = 11; j > 0; j--) {
                state.codeBuffer[j] = state.codeBuffer[j - 1];
            }
            state.codeBuffer[0] = i;
        } else {
            state.pressed[i] = 0;
        }
        if (bitRead(state.snesRegister, i) == UNPRESSED && bitRead(state.snesRegOld, i) == PRESSED) {
            state.released[i] = 1;
        } else {
            state.released[i] = 0;
        }
        if (bitRead(state.snesRegister, i) == PRESSED && bitRead(state.snesRegOld, i) == PRESSED) {
            state.held[i] = 1;
        } else {
            state.held[i] = 0;
        }
        if (bitRead(state.snesRegister, i) == UNPRESSED && bitRead(state.snesRegOld, i) == UNPRESSED) {
            state.unpressed[i] = 1;
        } else {
            state.unpressed[i] = 0;
        }
    }

    // Update orderHeld (tracks the order of button presses)
    for (int i = 0; i <= 11; i++) {
        if (state.pressed[i] == 1) {
            // Find the highest current orderHeld value
            int maxOrder = 0;
            for (int j = 0; j <= 11; j++) {
                if (state.orderHeld[j] > maxOrder) {
                    maxOrder = state.orderHeld[j];
                }
            }
            // If the button is already held, we need to adjust the order of other buttons
            if (state.orderHeld[i] > 0) {
                // Decrement the order of buttons with higher order than this button
                for (int j = 0; j <= 11; j++) {
                    if (state.orderHeld[j] > state.orderHeld[i]) {
                        state.orderHeld[j]--;
                    }
                }
            }
            // Set the order for the newly pressed button to the highest order + 1
            state.orderHeld[i] = maxOrder + 1;
        }
        if (state.released[i] == 1) {
            // Decrement the order for buttons with higher order than the released button
            for (int j = 0; j <= 11; j++) {
                if (state.orderHeld[j] > state.orderHeld[i]) {
                    state.orderHeld[j]--;
                }
            }
            state.orderHeld[i] = 0;
        }
    }
}