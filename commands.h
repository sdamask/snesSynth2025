// commands.h
// Header file for button command processing functions, handling SNES controller
// button combinations to update the synthesizer state (e.g., toggle portamento, cycle scales).

#ifndef COMMANDS_H
#define COMMANDS_H

#include "synth_state.h"

void checkCommands(SynthState& state);

#endif