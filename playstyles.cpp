// playstyles.cpp
// Implements playstyle functions for the SNES synthesizer, handling different modes
// of note playback (Monophonic, ChordButton, Polyphonic).

#include "playstyles.h"
#include "synth.h"
#include "chords.h"
#include "audio.h"
#include "utils.h"
#include "midi.h" // Include for MIDI functions
#include "midi_utils.h" // Add back for midiToPitchFloat
#include "button_defs.h" // Include for BTN_ defines
#include "synth_state.h" // Include for PROFILE_ defines
#include "debug.h"       // Include for DEBUG_DEBUG
#include <Arduino.h>

// Desired musical order: Down, Left, Up, Right, Select, Start, Y, B, X, A
const int buttonToMusicalPosition[MAX_NOTE_BUTTONS] = {
    7, // BTN_B (0)      -> musical position 7
    6, // BTN_Y (1)      -> musical position 6
    4, // BTN_SELECT (2) -> musical position 4
    5, // BTN_START (3)  -> musical position 5
    2, // BTN_UP (4)     -> musical position 2
    0, // BTN_DOWN (5)   -> musical position 0
    1, // BTN_LEFT (6)   -> musical position 1
    3, // BTN_RIGHT (7)  -> musical position 3
    9, // BTN_A (8)      -> musical position 9
    8  // BTN_X (9)      -> musical position 8
};

// Mapping for Thunderstruck Intro Profile
// Indices correspond to BTN_B(0), Y(1), Select(2), Start(3), Up(4), Down(5), Left(6), Right(7), A(8), X(9)
const int thunderstruckMidiNotes[MAX_NOTE_BUTTONS] = {
    79, // BTN_B -> G
    78, // BTN_Y -> F#
    75, // BTN_SELECT -> D#
    76, // BTN_START -> E
    71, // BTN_UP -> B (Open String)
    71, // BTN_DOWN -> B (Open String)
    71, // BTN_LEFT -> B (Open String)
    71, // BTN_RIGHT -> B (Open String)
    81, // BTN_A -> A
    80  // BTN_X -> G#
};

// Monophonic playstyle
void handleMonophonic(SynthState& state) {
    // Handle boogie mode first if enabled
    if (state.boogieModeEnabled) {
        // Update boogie trigger states based on current hold status
        state.boogieLActive = state.held[BTN_L];
        state.boogieRActive = state.held[BTN_R];
        
        // Let handleMidiClock handle the actual note playing and MIDI messages.
        // We don't need to explicitly stop notes here, handleMidiClock will send NoteOff.
        return; 
    }

    // --- Standard Monophonic Mode Handling --- 
    bool currentButtonPressed = false;
    bool currentButtonReleased = false;
    int buttonToPlay = -1;

    // Check for currently pressed button (0-9)
    for (int i = 0; i < MAX_NOTE_BUTTONS; i++) {
        if (state.pressed[i]) {
            currentButtonPressed = true;
            buttonToPlay = i;
            break;
        }
    }

    // Check for released button (the one currently playing)
    if (state.currentButton != -1 && state.released[state.currentButton]) {
         currentButtonReleased = true;
    }
    
    // --- Handle Button Press --- 
    if (currentButtonPressed) {
        // Send Note Off for the previous note, if one was playing
        if (state.currentMidiNote != -1) {
            sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
            // Don't call stopNote(0) here, playNote handles voice reuse/portamento
        }

        // Calculate pitch bend based on L/R triggers
        int pitchBend = 0;
        // Standard profile: L=-12, R=+12, L+R=0 (Thunderstruck profile L button handled separately)
        if (state.customProfileIndex != PROFILE_THUNDERSTRUCK) {
            if (state.held[BTN_L] && state.held[BTN_R]) pitchBend = 0;
            else if (state.held[BTN_L]) pitchBend = -12;
            else if (state.held[BTN_R]) pitchBend = 12;
        } else { // Thunderstruck: Only R gives pitch bend
             if (state.held[BTN_R]) pitchBend = 12;
        }

        // Get base MIDI note
        int baseMidiNote = -1; 
        if (state.customProfileIndex == PROFILE_THUNDERSTRUCK) {
            if (buttonToPlay == BTN_L) { // Handle L button specifically in TS profile
                 baseMidiNote = 71; // Open B
            } else if (buttonToPlay >= 0 && buttonToPlay < MAX_NOTE_BUTTONS) {
                 baseMidiNote = thunderstruckMidiNotes[buttonToPlay];
            } else { return; } // Invalid button index
        } else { // Standard Scale Profile
             if (buttonToPlay >= 0 && buttonToPlay < MAX_NOTE_BUTTONS) {
                 int musicalPosition = buttonToMusicalPosition[buttonToPlay];
                 baseMidiNote = state.scaleHolder[musicalPosition];
             } else { return; } // Invalid button index
        }

        // Apply pitch bend and play note
        if (baseMidiNote != -1) {
            int midiNote = baseMidiNote + pitchBend;
            if (midiNote < 0) midiNote = 0;
            if (midiNote > 127) midiNote = 127;

            playNote(state, 0, midiNote); // Play internal sound
            sendMidiNoteOn(midiNote, MIDI_VELOCITY, MIDI_CHANNEL); // Send MIDI Note On
            
            // Update state
            state.currentNote = midiNote; // Deprecated? Use currentMidiNote
            state.currentMidiNote = midiNote; // Store the MIDI note being played
            state.currentButton = buttonToPlay;
            state.currentFrequency = midiToPitchFloat[midiNote]; // Use lookup from utils.h
        }
    }
    // --- Handle Button Release --- 
    else if (currentButtonReleased) {
        // Send Note Off for the released note
        if (state.currentMidiNote != -1) {
            sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
        }
        stopNote(0); // Stop internal sound
        
        // Reset state
        state.currentNote = -1;
        state.currentMidiNote = -1; // Mark no MIDI note playing
        state.currentButton = -1;
        state.currentFrequency = 0.0;
    }
}

// ChordButton playstyle
void handleChordButton(SynthState& state) {
    // --- Determine current input states --- (Pitch Bend, New Press, Release, Pitch Change)
    int currentLState = state.held[BTN_L];
    int currentRState = state.held[BTN_R];
    int newPitchBend;
    if (currentLState == 1 && currentRState == 1) newPitchBend = 0;
    else if (currentLState == 1) newPitchBend = -12;
    else if (currentRState == 1) newPitchBend = 12;
    else newPitchBend = 0;

    int newlyPressedButton = -1; 
    for (int btnIndex = 0; btnIndex < 10; ++btnIndex) {
        if (state.pressed[btnIndex]) { newlyPressedButton = btnIndex; break; }
    }
    bool currentButtonReleased = (state.currentButton != -1 && state.released[state.currentButton]);
    bool pitchBendChanged = (state.currentButton != -1 && newPitchBend != state.prevPitchBend);
    
    // --- Determine Next Action --- 
    bool shouldStopNotes = false; 
    bool triggerNewChord = false; 
    int buttonToPlay = state.currentButton; 

    if (newlyPressedButton != -1) {
        triggerNewChord = true;
        buttonToPlay = newlyPressedButton;
    } else if (currentButtonReleased) {
        // --- Chord Retrigger Logic (Similar to Mono) ---
        // 1. Try buffer
        int lastHeldButtonInBuffer = -1;
        int readIndex = state.lastPressedIndex; 
        for (int i = 0; i < LAST_PRESS_BUFFER_SIZE; ++i) {
            readIndex = (readIndex + LAST_PRESS_BUFFER_SIZE - 1) % LAST_PRESS_BUFFER_SIZE;
            int bufferedButton = state.lastPressedBuffer[readIndex];
            if (bufferedButton >= 0 && bufferedButton < MAX_NOTE_BUTTONS && 
                state.held[bufferedButton] && bufferedButton != state.currentButton) { 
                lastHeldButtonInBuffer = bufferedButton;
                break; 
            }
        }
        if (lastHeldButtonInBuffer != -1) {
            Serial.print("Retriggering chord (Buffer Hit) for button: "); Serial.println(lastHeldButtonInBuffer);
            triggerNewChord = true;
            buttonToPlay = lastHeldButtonInBuffer;
        } else {
            // 2. Fallback: lowest index held
            int lowestFallbackHeld = -1;
            for (int btnIndex = 0; btnIndex < MAX_NOTE_BUTTONS; ++btnIndex) {
                 if (state.held[btnIndex] && btnIndex != state.currentButton) { 
                     lowestFallbackHeld = btnIndex;
                     break; 
                 }
            }
            if (lowestFallbackHeld != -1) {
                 Serial.print("Retriggering chord (Fallback Held - Lowest) for button: "); Serial.println(lowestFallbackHeld);
                 triggerNewChord = true;
                 buttonToPlay = lowestFallbackHeld;
            } else {
                 // 3. Nothing else held, stop.
                 shouldStopNotes = true;
            }
        }
    } else if (pitchBendChanged) {
        triggerNewChord = true;
        // buttonToPlay remains state.currentButton
    } else {
        // Check if all buttons released
        bool anyNoteHeld = false;
        for (int btnIndex = 0; btnIndex < 10; ++btnIndex) { if (state.held[btnIndex]) { anyNoteHeld = true; break; } }
        if (!anyNoteHeld && state.currentButton != -1) { shouldStopNotes = true; }
    }

    // --- Execute Action --- 
    if (shouldStopNotes) {
        // --- Stop Chord ---
        if (state.currentButton != -1) { 
             Serial.println("Stopping chord (button released w/o retrigger or none held)");
            bool notesWerePlaying = false;
            for (int i = 0; i < 4; i++) {
                if (state.currentChordNotes[i] != -1) {
                    notesWerePlaying = true;
                    stopNote(i);
                    sendMidiNoteOff(state.currentChordNotes[i], 0, MIDI_CHANNEL); 
                    state.currentChordNotes[i] = -1;
                    state.currentChordFrequencies[i] = 0.0;
                    state.waveformOpen[i] = 1;
                }
            }
            // Send MIDI All Notes Off if any notes were stopped
            if (notesWerePlaying) {
                usbMIDI.sendControlChange(123, 0, MIDI_CHANNEL);
                usbMIDI.send_now();
            }
            state.currentButton = -1; 
        }
    } else if (triggerNewChord && buttonToPlay != -1) { 
        // --- Play / Retrigger Chord --- 
        bool isNewButton = (newlyPressedButton != -1 && newlyPressedButton != state.currentButton);

        // Prepare previous voices: Send MIDI Note Offs. Stop audio voices only if Portamento is OFF.
        if (state.currentButton != -1 && (isNewButton || pitchBendChanged)) {
            Serial.println("Preparing voices for new/changed chord...");
             for (int i = 0; i < 4; i++) {
                if (state.currentChordNotes[i] != -1) {
                     sendMidiNoteOff(state.currentChordNotes[i], 0, MIDI_CHANNEL); 
                     
                     // *** CORRECTED STOP LOGIC ***
                     // Only stop audio voice if Portamento is OFF.
                     if (!state.portamentoEnabled) {
                         Serial.println("   Stopping voice (Porta OFF)");
                         stopNote(i);
                         state.currentChordNotes[i] = -1; 
                         state.currentChordFrequencies[i] = 0.0;
                         state.waveformOpen[i] = 1;
                     } else {
                         Serial.println("   Porta ON: Keeping voice active for slide.");
                         // Keep voice active for slide, state will be updated below
                     }
                }
            }
        }
        
        state.currentButton = buttonToPlay; 

        // Get the chord notes 
        int musicalPosition = buttonToMusicalPosition[state.currentButton];
        int chordNotes[4];
        int numNotes;
        getChordNotes(state, musicalPosition + 1, chordNotes, numNotes); 

        Serial.print("Playing new chord for button "); Serial.print(state.currentButton); Serial.print(" (musical pos "); Serial.print(musicalPosition); Serial.println(")");
        
        // Play each note in the new chord
        for (int i = 0; i < numNotes; i++) {
            if (chordNotes[i] != -1) {
                int finalMidiNote = chordNotes[i] + newPitchBend; 
                if (finalMidiNote < 0) finalMidiNote = 0;
                if (finalMidiNote > 127) finalMidiNote = 127;
                
                Serial.print("  Voice "); Serial.print(i); Serial.print(": MIDI "); Serial.println(finalMidiNote);
                playNote(state, i, finalMidiNote); 
                sendMidiNoteOn(finalMidiNote, MIDI_VELOCITY, MIDI_CHANNEL);
                state.currentChordNotes[i] = finalMidiNote;
                state.currentChordFrequencies[i] = midiToPitchFloat[finalMidiNote];
                state.waveformOpen[i] = 0;
            } 
        }
        // Stop unused voices
         for (int i = numNotes; i < 4; ++i) {
              if (state.currentChordNotes[i] != -1) {
                   Serial.print("  Stopping unused voice "); Serial.println(i);
                   stopNote(i);
                   sendMidiNoteOff(state.currentChordNotes[i], 0, MIDI_CHANNEL);
                   state.currentChordNotes[i] = -1;
                   state.currentChordFrequencies[i] = 0.0;
                   state.waveformOpen[i] = 1;
              }
         }
    }

    // --- Update State for Next Cycle ---
    state.pitchBend = newPitchBend; 
    state.prevPitchBend = newPitchBend; 
}


// Placeholder for Polyphonic playstyle
void handlePolyphonic(SynthState& state) {
    // To be implemented
    // Will need to iterate through state.pressed[] and state.released[] for buttons 0-9
    // Map BTN_ index to musical position using buttonToMusicalPosition
    // Assign notes to available voices
    // Handle note offs
}