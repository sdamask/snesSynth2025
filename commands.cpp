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
    else if (command.startsWith("waveform")) {
        // Format: waveform <type_index>
        int newWaveform = command.substring(9).toInt();
        // Assuming 4 waveforms for now (0-3)
        if (newWaveform >= 0 && newWaveform < 4) { 
            state.currentWaveform = newWaveform;
            DEBUG_INFO(CAT_COMMAND, "Waveform changed to %d", newWaveform);
            // Apply the change immediately (optional, depends on where waveform is set)
            // applyWaveformChange(state); // We'll handle this in playNote or audio setup
        } else {
            DEBUG_WARNING(CAT_COMMAND, "Invalid waveform index: %d", newWaveform);
        }
    }
    else if (command.startsWith("vibrato rate")) {
        int newRate = command.substring(13).toInt();
        if (newRate >= 0 && newRate <= 2) { // 0=Off, 1=5Hz, 2=10Hz
            state.vibratoRate = newRate;
            DEBUG_INFO(CAT_COMMAND, "Vibrato Rate set to %d", newRate);
            // Apply change later in audio code
        } else {
             DEBUG_WARNING(CAT_COMMAND, "Invalid vibrato rate index: %d", newRate);
        }
    }
    else if (command.startsWith("vibrato depth")) {
        int newDepth = command.substring(14).toInt();
        if (newDepth >= 0 && newDepth <= 3) { // 0=Off, 1=Low, 2=Medium, 3=High
            state.vibratoDepth = newDepth;
            DEBUG_INFO(CAT_COMMAND, "Vibrato Depth set to %d", newDepth);
             // Apply change later in audio code
        } else {
             DEBUG_WARNING(CAT_COMMAND, "Invalid vibrato depth index: %d", newDepth);
        }
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

    // Check for L + R + B to cycle Waveforms
    // Indices: BTN_L=10, BTN_R=11, BTN_B=0
    if (state.held[BTN_L] && state.held[BTN_R] && state.held[BTN_B]) { 
        // Trigger only when B is newly pressed while L and R are already held
        if (!state.prevHeld[BTN_B]) {
            state.currentWaveform = (state.currentWaveform + 1) % 4; // Cycle through 0, 1, 2, 3
            const char* waveformNames[] = {"Sine", "Sawtooth", "Square", "Triangle"};
            DEBUG_INFO(CAT_COMMAND, "Waveform changed to %d (%s) via button combo", state.currentWaveform, waveformNames[state.currentWaveform]);
            Serial.print("COMMAND: Waveform set to "); Serial.println(waveformNames[state.currentWaveform]);
            state.commandJustExecuted = true; // Set flag
            // Apply the change immediately (optional)
            // applyWaveformChange(state); // We'll handle this later
        }
    }

    // Check for L + R + X to cycle Vibrato Depth (WAS Rate)
    // Indices: BTN_L=10, BTN_R=11, BTN_X=9
    if (state.held[BTN_L] && state.held[BTN_R] && state.held[BTN_X]) { 
        if (!state.prevHeld[BTN_X]) {
            state.vibratoDepth = (state.vibratoDepth + 1) % 4; // Cycle 0, 1, 2, 3 (WAS Rate)
            const char* depthNames[] = {"Off", "Low", "Medium", "High"};
            DEBUG_INFO(CAT_COMMAND, "Vibrato Depth changed to %d (%s) via button combo", state.vibratoDepth, depthNames[state.vibratoDepth]);
            Serial.print("COMMAND: Vibrato Depth set to "); Serial.println(depthNames[state.vibratoDepth]);
            state.commandJustExecuted = true; 
        }
    }

    // Check for L + R + Y to cycle Vibrato Rate (WAS Depth)
    // Indices: BTN_L=10, BTN_R=11, BTN_Y=1
    if (state.held[BTN_L] && state.held[BTN_R] && state.held[BTN_Y]) { 
        if (!state.prevHeld[BTN_Y]) {
            state.vibratoRate = (state.vibratoRate + 1) % 3; // Cycle 0, 1, 2 (WAS Depth)
            const char* rateNames[] = {"Off", "5Hz", "10Hz"};
            DEBUG_INFO(CAT_COMMAND, "Vibrato Rate changed to %d (%s) via button combo", state.vibratoRate, rateNames[state.vibratoRate]);
            Serial.print("COMMAND: Vibrato Rate set to "); Serial.println(rateNames[state.vibratoRate]);
            state.commandJustExecuted = true; 
        }
    }

    // Check for L + R + Select to cycle Mapping Profile
    // Indices: BTN_L=10, BTN_R=11, BTN_SELECT=2
    if (state.held[BTN_L] && state.held[BTN_R] && state.held[BTN_SELECT]) { 
        if (!state.prevHeld[BTN_SELECT]) {
            if (state.customProfileIndex == PROFILE_SCALE) {
                state.customProfileIndex = PROFILE_THUNDERSTRUCK;
                DEBUG_INFO(CAT_COMMAND, "Mapping Profile changed to Thunderstruck via button combo");
                Serial.println("COMMAND: Mapping Profile set to Thunderstruck");
            } else {
                state.customProfileIndex = PROFILE_SCALE;
                DEBUG_INFO(CAT_COMMAND, "Mapping Profile changed to Scale via button combo");
                Serial.println("COMMAND: Mapping Profile set to Scale");
            }
            state.commandJustExecuted = true; 
        }
    }
}