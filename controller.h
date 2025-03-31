// controller.h
// Header file for SNES controller functions, including setup and button state reading.

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "synth_state.h"
#include "button_defs.h"  // Include button definitions

// Pin definitions for SNES controller
#define SNES_DATA 4
#define SNES_CLOCK 2
#define SNES_LATCH 3

void setupController();
void buttonState(SynthState& state);

#endif