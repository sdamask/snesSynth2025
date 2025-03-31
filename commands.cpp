// commands.cpp
// Implements functions for processing SNES controller button combinations,
// updating the synthesizer state (e.g., toggle portamento, cycle scales, change key).

#include "commands.h"
#include "debug.h"
#include "utils.h"
#include "button_defs.h"

void handleSerialCommand(String command, SynthState& state) {
    // Remove any whitespace
    command.trim();
    
    if (command.startsWith("scale")) {
        // Format: scale <number>
        int newScale = command.substring(6).toInt();
        if (newScale >= 0 && newScale < 7) {  // 7 scales available
            state.scaleMode = newScale;
            state.needsScaleUpdate = true;
            DEBUG_INFO(CAT_COMMAND, "Scale mode changed to %d", newScale);
        }
    }
    else if (command.startsWith("base")) {
        // Format: base <note>
        int newBase = command.substring(5).toInt();
        if (newBase >= 36 && newBase <= 84) {  // Reasonable MIDI note range
            state.baseNote = newBase;
            state.needsScaleUpdate = true;
            DEBUG_INFO(CAT_COMMAND, "Base note changed to %d", newBase);
        }
    }
    else if (command.startsWith("key")) {
        // Format: key <offset>
        int newOffset = command.substring(4).toInt();
        if (newOffset >= 0 && newOffset < 12) {
            state.keyOffset = newOffset;
            state.needsScaleUpdate = true;
            DEBUG_INFO(CAT_COMMAND, "Key offset changed to %d", newOffset);
        }
    }
    else if (command == "mono") {
        state.playStyle = MONOPHONIC;
        DEBUG_INFO(CAT_COMMAND, "Play style set to monophonic");
    }
    else if (command == "poly") {
        state.playStyle = POLYPHONIC;
        DEBUG_INFO(CAT_COMMAND, "Play style set to polyphonic");
    }
    else if (command == "chord") {
        state.playStyle = CHORD_BUTTON;
        DEBUG_INFO(CAT_COMMAND, "Play style set to chord button");
    }
    else if (command == "portamento") {
        state.portamentoEnabled = !state.portamentoEnabled;
        DEBUG_INFO(CAT_COMMAND, "Portamento %s", state.portamentoEnabled ? "enabled" : "disabled");
    }
}

void checkCommands(SynthState& state) {
    // Check for L + R + A to toggle portamento
    if (state.held[BTN_L] && state.held[BTN_R] && state.held[BTN_A]) {
        // Trigger only when A is newly pressed while L and R are already held
        if (!state.prevHeld[BTN_A]) { 
            state.portamentoEnabled = !state.portamentoEnabled;
            DEBUG_INFO(CAT_COMMAND, "Portamento %s via button combo", state.portamentoEnabled ? "enabled" : "disabled");
            Serial.print("COMMAND: Portamento "); Serial.println(state.portamentoEnabled ? "ENABLED" : "DISABLED"); // Add confirmation
            state.commandJustExecuted = true; // Set flag
        }
    }
    
    // Check for L + R + Up to cycle play styles
    if (state.held[BTN_L] && state.held[BTN_R] && state.held[BTN_UP]) { 
        // Trigger only when Up is newly pressed while L and R are already held
        if (!state.prevHeld[BTN_UP]) {
            const char* newStyleName = "Unknown"; // Variable to hold the name of the new style
            switch (state.playStyle) {
                case MONOPHONIC:
                    state.playStyle = CHORD_BUTTON;  // Switch directly between mono and chord modes
                    newStyleName = "Chord Button";
                    DEBUG_INFO(CAT_COMMAND, "Play style changed to chord button");
                    break;
                case CHORD_BUTTON:
                    state.playStyle = MONOPHONIC;
                     newStyleName = "Monophonic";
                    DEBUG_INFO(CAT_COMMAND, "Play style changed to monophonic");
                    break;
                case POLYPHONIC:  // In case we're somehow in poly mode, go to mono
                    state.playStyle = MONOPHONIC;
                     newStyleName = "Monophonic";
                    DEBUG_INFO(CAT_COMMAND, "Play style changed to monophonic");
                    break;
            }
            Serial.print("COMMAND: Play Style set to "); Serial.println(newStyleName); // Add confirmation
            state.commandJustExecuted = true; // Set flag
        }
    }
}