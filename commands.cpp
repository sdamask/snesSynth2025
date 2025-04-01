// commands.cpp
// Implements functions for processing SNES controller button combinations,
// updating the synthesizer state (e.g., toggle portamento, cycle scales, change key).

#include "commands.h"
#include "debug.h"
#include "utils.h"
#include "button_defs.h"
#include "synth_state.h" // Needed for SynthState reference

void handleSerialCommand(String command, SynthState& state) {
    command.trim(); // Remove leading/trailing whitespace

    if (command.startsWith("scale")) {
        // Extract value after "scale "
        int scaleVal = command.substring(6).toInt();
        if (scaleVal >= 0 && scaleVal < 7) { // Assuming 7 scale modes (0-6)
            state.scaleMode = scaleVal;
            state.needsScaleUpdate = true;
            DEBUG_INFO(CAT_COMMAND, "Scale command: Set to %d", scaleVal);
        } else {
            DEBUG_WARNING(CAT_COMMAND, "Scale command: Invalid value %d", scaleVal);
        }
    } else if (command.startsWith("base")) {
        // Extract value after "base "
        int baseVal = command.substring(5).toInt();
        if (baseVal >= 36 && baseVal <= 84) { // Range check for base note
            state.baseNote = baseVal;
            state.needsScaleUpdate = true;
            DEBUG_INFO(CAT_COMMAND, "Base note command: Set to %d", baseVal);
        } else {
            DEBUG_WARNING(CAT_COMMAND, "Base note command: Invalid value %d", baseVal);
        }
    } else if (command.startsWith("offset")) {
        // Extract value after "offset "
        int offsetVal = command.substring(7).toInt();
        if (offsetVal >= 0 && offsetVal <= 11) { // Range check for offset
            state.keyOffset = offsetVal;
            state.needsScaleUpdate = true;
            DEBUG_INFO(CAT_COMMAND, "Offset command: Set to %d", offsetVal);
        } else {
            DEBUG_WARNING(CAT_COMMAND, "Offset command: Invalid value %d", offsetVal);
        }
    } else if (command.startsWith("debug")) {
        // Format: debug <CATEGORY_NAME> <LEVEL_NAME>
        // Format: debug global <LEVEL_NAME>
        int firstSpace = command.indexOf(' ');
        int secondSpace = command.indexOf(' ', firstSpace + 1);
        
        if (firstSpace != -1 && secondSpace != -1) {
            String categoryStr = command.substring(firstSpace + 1, secondSpace);
            String levelStr = command.substring(secondSpace + 1);
            categoryStr.toUpperCase();
            levelStr.toUpperCase();

            DebugLevel level = LEVEL_OFF; // Default to OFF
            if (levelStr == "ERROR") level = LEVEL_ERROR;
            else if (levelStr == "WARNING") level = LEVEL_WARNING;
            else if (levelStr == "INFO") level = LEVEL_INFO;
            else if (levelStr == "DEBUG") level = LEVEL_DEBUG;
            else if (levelStr == "VERBOSE") level = LEVEL_VERBOSE; // Handle VERBOSE
            else if (levelStr == "OFF") level = LEVEL_OFF; // Handle OFF

            if (categoryStr == "GLOBAL") {
                setGlobalDebugLevel(level); // Set level for all categories
            } else {
                // Find category index
                int categoryIndex = -1;
                for (int i = 0; i < CAT_COUNT; i++) {
                    if (categoryStr == categoryNames[i]) { // Now references the extern array
                        categoryIndex = i;
                        break;
                    }
                }
                if (categoryIndex != -1) {
                    setDebugLevelForCategory((DebugCategory)categoryIndex, level);
                } else {
                    DEBUG_WARNING(CAT_COMMAND, "Debug command: Invalid category '%s'", categoryStr.c_str());
                }
            }
        } else {
            DEBUG_WARNING(CAT_COMMAND, "Debug command: Invalid format");
        }
     } else if (command == "mono") {
        state.playStyle = MONOPHONIC;
        DEBUG_INFO(CAT_COMMAND, "Play style set to monophonic");
    } else if (command == "poly") {
        state.playStyle = POLYPHONIC;
        DEBUG_INFO(CAT_COMMAND, "Play style set to polyphonic");
    } else if (command == "chord") {
        state.playStyle = CHORD_BUTTON;
        DEBUG_INFO(CAT_COMMAND, "Play style set to chord button");
    } else if (command == "portamento") {
        state.portamentoEnabled = !state.portamentoEnabled;
        DEBUG_INFO(CAT_COMMAND, "Portamento %s", state.portamentoEnabled ? "enabled" : "disabled");
    } else if (command.startsWith("waveform")) {
        int newWaveform = command.substring(9).toInt();
        if (newWaveform >= 0 && newWaveform < 4) { 
            state.currentWaveform = newWaveform;
            DEBUG_INFO(CAT_COMMAND, "Waveform changed to %d", newWaveform);
        } else {
            DEBUG_WARNING(CAT_COMMAND, "Invalid waveform index: %d", newWaveform);
        }
    } else if (command.startsWith("vibrato rate")) {
        int newRate = command.substring(13).toInt();
        if (newRate >= 0 && newRate <= 2) { 
            state.vibratoRate = newRate;
            DEBUG_INFO(CAT_COMMAND, "Vibrato Rate set to %d", newRate);
        } else {
             DEBUG_WARNING(CAT_COMMAND, "Invalid vibrato rate index: %d", newRate);
        }
    } else if (command.startsWith("vibrato depth")) {
        int newDepth = command.substring(14).toInt();
        if (newDepth >= 0 && newDepth <= 3) { 
            state.vibratoDepth = newDepth;
            DEBUG_INFO(CAT_COMMAND, "Vibrato Depth set to %d", newDepth);
        } else {
             DEBUG_WARNING(CAT_COMMAND, "Invalid vibrato depth index: %d", newDepth);
        }
    } else if (command.startsWith("boogie_r_tick")) {
        int tickValue = command.substring(14).toInt(); // Get value after "boogie_r_tick "
        // Add reasonable range check (e.g., 12 to 20, matching GUI slider)
        if (tickValue >= 12 && tickValue <= 20) {
            state.boogieRTickValue = tickValue;
            DEBUG_INFO(CAT_COMMAND, "Boogie R Tick set to %d", state.boogieRTickValue);
        } else {
            DEBUG_WARNING(CAT_COMMAND, "Invalid Boogie R Tick value: %d", tickValue);
        }
    } else {
        DEBUG_WARNING(CAT_COMMAND, "Unknown command: %s", command.c_str());
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

    // Check for L+R+Select (Toggle Mapping Profile)
    if (state.held[BTN_L] && state.held[BTN_R] && state.pressed[BTN_SELECT]) {
        state.customProfileIndex = (state.customProfileIndex == PROFILE_SCALE) ? PROFILE_THUNDERSTRUCK : PROFILE_SCALE;
        DEBUG_DEBUG(CAT_COMMAND, "Toggling Mapping Profile: %s", state.customProfileIndex == PROFILE_SCALE ? "Scale" : "Thunderstruck");
        Serial.printf("Switched to %s Mapping\n", state.customProfileIndex == PROFILE_SCALE ? "Scale" : "Thunderstruck");
        state.commandJustExecuted = true;
        return;
    }

    // Check for L+R+Start (Toggle Boogie Mode)
    if (state.held[BTN_L] && state.held[BTN_R] && state.pressed[BTN_START]) {
        state.boogieModeEnabled = !state.boogieModeEnabled;
        DEBUG_DEBUG(CAT_COMMAND, "Toggling Boogie Mode: %s", state.boogieModeEnabled ? "ON" : "OFF");
        Serial.printf("Boogie Mode: %s\n", state.boogieModeEnabled ? "ON" : "OFF");
        state.commandJustExecuted = true;
        return;
    }
}