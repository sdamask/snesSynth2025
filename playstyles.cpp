// playstyles.cpp
// Implements playstyle functions for the SNES synthesizer, handling different modes
// of note playback (Monophonic, ChordButton, Polyphonic).

#include "playstyles.h"
#include "synth.h"
#include "chords.h"
#include "audio.h"
#include "midi_utils.h"
#include "button_defs.h" // Include for BTN_ defines
#include <Arduino.h>

// Desired musical order: Down, Left, Up, Right, Select, Start, Y, B, X, A
const int buttonToMusicalPosition[10] = {
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

// Monophonic playstyle
void handleMonophonic(SynthState& state) {
    // Update pitchBend based on L (BTN_L) and R (BTN_R) states
    int currentLState = state.held[BTN_L];
    int currentRState = state.held[BTN_R];

    // Set pitchBend based on the current state of L and R
    int newPitchBend;
    if (currentLState == 1 && currentRState == 1) {
        newPitchBend = 0;
    } else if (currentLState == 1) {
        newPitchBend = -12;
    } else if (currentRState == 1) {
        newPitchBend = 12;
    } else {
        newPitchBend = 0;
    }

    // REMOVED: Early Note Off/State Update
    /*
    if (newPitchBend != state.pitchBend && state.currentMidiNote != -1) {
        sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
    }
    state.pitchBend = newPitchBend;
    */

    // --- Detect Trigger Conditions ---
    
    // Find the *newly* pressed button (BTN_ index 0-9)
    int newlyPressedButton = -1; 
    for (int btnIndex = 0; btnIndex < MAX_NOTE_BUTTONS; ++btnIndex) { // Use MAX_NOTE_BUTTONS
        if (state.pressed[btnIndex]) {
             newlyPressedButton = btnIndex; 
             break; // Prioritize first new press found
        }
    }

    // Check if the button currently assigned to the note was released
    bool currentButtonReleased = (state.currentButton != -1 && state.released[state.currentButton]);
    
    // Check if pitch bend changed while a note was potentially playing
    // CORRECTED: Compare newPitchBend with state.prevPitchBend
    bool pitchBendChanged = (state.currentButton != -1 && newPitchBend != state.prevPitchBend);

    // --- Determine Next Action ---
    
    int buttonToPlay = -1; // BTN_ index of the note to play next, if any (-1 means stop/do nothing)
    bool stopCurrentNote = false;

    if (newlyPressedButton != -1) {
        // New button press takes priority
        buttonToPlay = newlyPressedButton;
    } else if (currentButtonReleased) {
        // Current note's button was released. Find the most recently pressed *other* held button from the buffer.
        int lastHeldButtonInBuffer = -1;
        int readIndex = state.lastPressedIndex; // Start checking from the last written index
        for (int i = 0; i < LAST_PRESS_BUFFER_SIZE; ++i) {
            readIndex = (readIndex + LAST_PRESS_BUFFER_SIZE - 1) % LAST_PRESS_BUFFER_SIZE; // Decrement index with wrap (read backwards)
            int bufferedButton = state.lastPressedBuffer[readIndex];
            // Check if the button from buffer is valid (0-9), is currently held, and is NOT the one just released
            if (bufferedButton >= 0 && bufferedButton < MAX_NOTE_BUTTONS && 
                state.held[bufferedButton] && 
                bufferedButton != state.currentButton) { 
                lastHeldButtonInBuffer = bufferedButton;
                break; // Found the most recent valid one still held
            }
        }
        
        if (lastHeldButtonInBuffer != -1) {
            // Retrigger with the last held button from buffer
            Serial.print("Retriggering note (Buffer Hit) for button: "); Serial.println(lastHeldButtonInBuffer);
            buttonToPlay = lastHeldButtonInBuffer;
        } else {
            // Buffer scan failed. Fallback: Find the *lowest* index button (0-9)
            // that is still held and wasn't the one just released.
            int lowestFallbackHeld = -1;
            for (int btnIndex = 0; btnIndex < MAX_NOTE_BUTTONS; ++btnIndex) {
                 if (state.held[btnIndex] && btnIndex != state.currentButton) { 
                     lowestFallbackHeld = btnIndex;
                     break; // Found the lowest index one still held
                 }
            }
            
            if (lowestFallbackHeld != -1) {
                 // Play the lowest index held button as fallback
                 Serial.print("Retriggering note (Fallback Held - Lowest) for button: "); Serial.println(lowestFallbackHeld);
                 buttonToPlay = lowestFallbackHeld;
            } else {
                 // Buffer failed AND no *other* buttons are held. Stop the note.
                 stopCurrentNote = true;
            }
        }
    } else if (pitchBendChanged) {
        // Only pitch bend changed, keep playing the current button but update pitch
        buttonToPlay = state.currentButton; 
    } else {
        // No new press, no release of current, no pitch bend change.
        // Check if *any* note buttons are still held
        bool anyNoteHeld = false;
        for (int btnIndex = 0; btnIndex < MAX_NOTE_BUTTONS; ++btnIndex) {
            if (state.held[btnIndex]) {
                anyNoteHeld = true;
                break;
            }
        }
        if (!anyNoteHeld && state.currentButton != -1) {
            // All buttons previously held are now released
            stopCurrentNote = true;
        }
    }

    // --- Execute Action ---
    
    if (stopCurrentNote) {
        if (state.currentButton != -1) {  // Only stop if we were playing something
             Serial.println("Stopping note (no buttons held or last released without retrigger)");
             stopNote(0);  // Use voice 0
             if (state.currentMidiNote != -1) {
                 sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
             }
             state.currentButton = -1;
             state.currentMidiNote = -1;
             state.currentFrequency = 0.0;
             state.waveformOpen[0] = 1;
        }
    } else if (buttonToPlay != -1) {
        // A button should be playing (either new press or retrigger)
        // state.pitchBend = newPitchBend; // Update pitch bend state regardless -- MOVED TO END?

        // Map the BTN_ index to its musical position
        int musicalPosition = buttonToMusicalPosition[buttonToPlay];
        
        // Compute the MIDI note for the new musical position, using the LATEST pitchBend value
        int midiNote = state.scaleHolder[musicalPosition] + newPitchBend; // USE newPitchBend HERE
        if (midiNote < 0) midiNote = 0;
        if (midiNote > 127) midiNote = 127;

        // Determine if a Note On is needed:
        bool triggerNoteOn = (state.currentMidiNote != midiNote) || (state.currentButton != buttonToPlay);

        // Send MIDI Note Off for the previous note ONLY if Note On will be triggered
        if (state.currentMidiNote != -1 && triggerNoteOn) {
             sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
        }

        // Play the new note (or update pitch) if required
        if (triggerNoteOn) {
             // Add debug print for pitch bend retrigger
             if (pitchBendChanged && newlyPressedButton == -1 && !currentButtonReleased) {
                 Serial.println("-----> Retriggering due to Pitch Bend Change <-----");
                 Serial.print("       Old MIDI Note: "); Serial.println(state.currentMidiNote);
                 Serial.print("       New MIDI Note: "); Serial.println(midiNote);
                 Serial.print("       Current Button: "); Serial.println(state.currentButton);
                 Serial.print("       Button To Play: "); Serial.println(buttonToPlay);
                 Serial.print("       Prev Pitch Bend: "); Serial.println(state.prevPitchBend);
                 Serial.print("       New Pitch Bend: "); Serial.println(newPitchBend);
             }

             // Send MIDI Note On
             sendMidiNoteOn(midiNote, MIDI_VELOCITY, MIDI_CHANNEL);
             state.waveformOpen[0] = 0;

             Serial.print("Playing new note (Mono): "); Serial.print(midiNote); Serial.print(" for button "); Serial.println(buttonToPlay);
             playNote(state, 0, midiNote);  // Use voice 0
             state.currentFrequency = midiToPitchFloat[midiNote];
             state.currentMidiNote = midiNote; // Update the currently playing note
        }
        // Update the button associated with the playing note
        state.currentButton = buttonToPlay; 
    }
    
    // Update pitchBend state and store previous value for next cycle
    state.pitchBend = newPitchBend; 
    state.prevPitchBend = newPitchBend; // Store the *new* value as previous for the next loop
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
            for (int i = 0; i < 4; i++) {
                if (state.currentChordNotes[i] != -1) {
                    stopNote(i);
                    sendMidiNoteOff(state.currentChordNotes[i], 0, MIDI_CHANNEL); 
                    state.currentChordNotes[i] = -1;
                    state.currentChordFrequencies[i] = 0.0;
                    state.waveformOpen[i] = 1;
                }
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