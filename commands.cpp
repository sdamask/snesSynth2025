// commands.cpp
// Implements functions for processing SNES controller button combinations,
// updating the synthesizer state (e.g., toggle portamento, cycle scales, change key).

#include "commands.h"
#include "debug.h"
#include "utils.h"
#include "button_defs.h"
#include "synth_state.h" // Needed for SynthState reference
#include "midi.h" // Add for sendMidiNoteOff, MIDI_CHANNEL
#include "audio.h" // Add for stopNote

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
    } else if (command.startsWith("pattern")) {
        // Format: pattern <numNotes> <totalTicks>
        int firstSpace = command.indexOf(' ');
        int secondSpace = command.indexOf(' ', firstSpace + 1);
        
        if (firstSpace != -1 && secondSpace != -1) {
            // Extract values
            int numNotes = command.substring(firstSpace + 1, secondSpace).toInt();
            float totalTicks = command.substring(secondSpace + 1).toFloat();

            // Validate values
            if (numNotes >= 1 && numNotes <= state.MAX_PATTERN_NOTES && totalTicks > 0.1f) { // Basic validation
                 DEBUG_INFO(CAT_COMMAND, "Pattern command received: N=%d, TotalTicks=%.2f", numNotes, totalTicks);
                 
                 // Update state
                 state.numNotesInPattern = numNotes;
                 state.currentRhythmPatternLengthTicks = totalTicks;
                 float ticksPerNote = state.currentRhythmPatternLengthTicks / state.numNotesInPattern;
                 
                 DEBUG_INFO(CAT_COMMAND, "Recalculating pattern: %.2f ticks per note", ticksPerNote);
                 
                 for (int i = 0; i < state.numNotesInPattern; ++i) {
                     state.currentRhythmPatternTicks[i] = i * ticksPerNote;
                     state.notePlayedInCycle[i] = false; // Reset played flags
                     DEBUG_VERBOSE(CAT_COMMAND, "  Pattern[%d] = %.2f ticks", i, state.currentRhythmPatternTicks[i]);
                 }
                 // Clear remaining slots
                 for (int i = state.numNotesInPattern; i < state.MAX_PATTERN_NOTES; ++i) {
                     state.currentRhythmPatternTicks[i] = 0.0f;
                     state.notePlayedInCycle[i] = false;
                 }
                 
                 // Reset cycle timing immediately to use new pattern
                 state.cycleStartTimeMicros = micros(); 
                 state.lastTickTimeMicros = micros(); // Prevent large tick duration on next clock
                 for(int i = 0; i < state.numNotesInPattern; ++i) { state.notePlayedInCycle[i] = false; }

            } else {
                 DEBUG_WARNING(CAT_COMMAND, "Pattern command: Invalid values N=%d, TotalTicks=%.2f", numNotes, totalTicks);
            }
        } else {
             DEBUG_WARNING(CAT_COMMAND, "Pattern command: Invalid format '%s'", command.c_str());
        }
    } else if (command.startsWith("boogie_ratio")) {
        // Format: boogie_ratio <float_value>
        float ratioValue = command.substring(13).toFloat(); // Get value after "boogie_ratio "
        // Add some basic validation (e.g., 0.0 to 1.0)
        if (ratioValue >= 0.0f && ratioValue <= 1.0f) { 
            state.boogieRTimingRatio = ratioValue;
            DEBUG_INFO(CAT_COMMAND, "Boogie R Timing Ratio set to %.2f", state.boogieRTimingRatio);
        } else {
            DEBUG_WARNING(CAT_COMMAND, "Invalid Boogie R Timing Ratio value: %.2f", ratioValue);
        }
    } else if (command.startsWith("mode")) {
        String modeName = command.substring(5); // Get name after "mode "
        modeName.toLowerCase(); // Consistent comparison
        if (modeName == "standard") {
            state.boogieModeEnabled = false;
            state.rhythmicModeEnabled = false;
            DEBUG_INFO(CAT_COMMAND, "Mode set to Standard");
        } else if (modeName == "boogie") {
            state.boogieModeEnabled = true;
            state.rhythmicModeEnabled = false;
            DEBUG_INFO(CAT_COMMAND, "Mode set to Boogie");
        } else if (modeName == "rhythmic") {
            state.boogieModeEnabled = false;
            state.rhythmicModeEnabled = true;
            DEBUG_INFO(CAT_COMMAND, "Mode set to Rhythmic");
        } else {
            DEBUG_WARNING(CAT_COMMAND, "Unknown mode: %s", modeName.c_str());
        }
        // Stop notes from previous mode when changing via GUI
        if (state.boogieCurrentMidiNote != -1) { DEBUG_VERBOSE(CAT_MIDI, "Stopping Boogie note on mode change (GUI)"); sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL); stopNote(0); state.boogieCurrentMidiNote = -1; state.boogieTriggerButton = -1; state.boogieCurrentSlotIndex = -1; }
        if (state.lastRhythmicMidiNote != -1) { DEBUG_VERBOSE(CAT_MIDI, "Stopping Rhythmic note on mode change (GUI)"); sendMidiNoteOff(state.lastRhythmicMidiNote, 0, MIDI_CHANNEL); stopNote(0); state.lastRhythmicMidiNote = -1; }
    } else if (strncmp(command.c_str(), "set mode ", 9) == 0) {
        int modeVal = atoi(command.c_str() + 9);
        if (modeVal >= 0 && modeVal < NUM_SCALES) { // Use NUM_SCALES defined in synth.h
            state.scaleMode = modeVal;
            state.needsScaleUpdate = true; // Trigger scale recalculation
            Serial.printf("COMMAND: Scale Mode set to %d\n", state.scaleMode);
            DEBUG_INFO(CAT_COMMAND, "Scale mode command: Set to %d", state.scaleMode);
        } else {
            Serial.printf("ERROR: Invalid scale mode value %d (Valid: 0-%d)\n", modeVal, NUM_SCALES - 1);
            DEBUG_WARNING(CAT_COMMAND, "Scale mode command: Invalid value %d", modeVal);
        }
        state.commandJustExecuted = true;
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

    // Check for L+R+Select (Toggle Mapping Profile)
    if (state.held[BTN_L] && state.held[BTN_R] && state.pressed[BTN_SELECT]) {
        state.customProfileIndex = (state.customProfileIndex == PROFILE_SCALE) ? PROFILE_THUNDERSTRUCK : PROFILE_SCALE;
        DEBUG_DEBUG(CAT_COMMAND, "Toggling Mapping Profile: %s", state.customProfileIndex == PROFILE_SCALE ? "Scale" : "Thunderstruck");
        Serial.printf("Switched to %s Mapping\n", state.customProfileIndex == PROFILE_SCALE ? "Scale" : "Thunderstruck");
        state.commandJustExecuted = true;
        return; // Ensure we exit after handling
    }

    // Check for L+R+Start (Cycle Play Mode: Standard / Boogie / Rhythmic)
    if (state.held[BTN_L] && state.held[BTN_R] && state.pressed[BTN_START]) {
        // Always cycle the mode regardless of MIDI clock status
        if (!state.boogieModeEnabled && !state.rhythmicModeEnabled) {
            // Currently Standard -> Switch to Boogie
            state.boogieModeEnabled = true;
            state.rhythmicModeEnabled = false;
            Serial.print("MODE: Boogie");
            if (!state.midiSyncEnabled) Serial.print(" (MIDI Clock Inactive)"); // Warn if inactive
            Serial.println();
        } else if (state.boogieModeEnabled) {
            // Currently Boogie -> Switch to Rhythmic
            state.boogieModeEnabled = false;
            state.rhythmicModeEnabled = true;
            Serial.print("MODE: Rhythmic Pattern");
            if (!state.midiSyncEnabled) Serial.print(" (MIDI Clock Inactive)"); // Warn if inactive
            Serial.println();
        } else { // Currently Rhythmic -> Switch to Standard
            state.boogieModeEnabled = false;
            state.rhythmicModeEnabled = false;
            Serial.println("MODE: Standard Play");
        }
        DEBUG_DEBUG(CAT_COMMAND, "Cycled Mode: Boogie=%d, Rhythmic=%d", state.boogieModeEnabled, state.rhythmicModeEnabled);
        
        // Ensure previous mode notes are stopped regardless of clock status
        if (state.boogieCurrentMidiNote != -1) { DEBUG_VERBOSE(CAT_MIDI, "Stopping Boogie note on mode change"); sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL); stopNote(0); state.boogieCurrentMidiNote = -1; state.boogieTriggerButton = -1; state.boogieCurrentSlotIndex = -1; }
        if (state.lastRhythmicMidiNote != -1) { DEBUG_VERBOSE(CAT_MIDI, "Stopping Rhythmic note on mode change"); sendMidiNoteOff(state.lastRhythmicMidiNote, 0, MIDI_CHANNEL); stopNote(0); state.lastRhythmicMidiNote = -1; }
        
        state.commandJustExecuted = true;
        return;
    }
}