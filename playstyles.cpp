// playstyles.cpp
// Implements playstyle functions for the SNES synthesizer, handling different modes
// of note playback (Monophonic, ChordButton, Polyphonic).

#include "playstyles.h"
#include "synth.h"
#include "chords.h"
#include "audio.h"
#include "midi_utils.h"
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
    // --- Update Pitch Bend based on L/R (Profile Dependent) ---
    int currentLState = state.held[BTN_L]; // L button state (used for note trigger in TS profile)
    int currentRState = state.held[BTN_R]; // R button state (used for pitch bend)
    int pitchBendOffset = 0; // Initialize to 0

    if (state.customProfileIndex == PROFILE_THUNDERSTRUCK) {
        // In Thunderstruck profile, only R gives pitch bend (+12)
        pitchBendOffset = currentRState ? 12 : 0;
    } else {
        // Standard profile: L=-12, R=+12, L+R=0
        if (currentLState == 1 && currentRState == 1) {
            pitchBendOffset = 0;
        } else if (currentLState == 1) {
            pitchBendOffset = -12;
        } else if (currentRState == 1) {
            pitchBendOffset = 12;
        } else {
            pitchBendOffset = 0;
        }
    }
    // Store the *calculated* offset to detect changes for retriggering
    int newPitchBend = pitchBendOffset;

    // --- Detect Trigger Conditions ---
    bool pitchBendChanged = (state.currentButton != -1 && newPitchBend != state.prevPitchBend);

    // Find the *newly* pressed button (BTN_ index 0-9 for standard notes)
    int newlyPressedButton = -1;
    for (int btnIndex = 0; btnIndex < MAX_NOTE_BUTTONS; ++btnIndex) { // Check 0-9 first
        if (state.pressed[btnIndex]) {
             newlyPressedButton = btnIndex;
             break; // Prioritize first new press found (0-9)
        }
    }

    // Check if the button currently assigned to the note was released (can be 0-9 or L)
    bool currentButtonReleased = (state.currentButton != -1 && state.released[state.currentButton]);

    // --- Determine Next Action ---
    int buttonToPlay = -1; // BTN_ index of the note to play next, if any (-1 means stop/do nothing)
    bool stopCurrentNote = false;

    if (newlyPressedButton != -1) {
        // New button (0-9) pressed, play it
        buttonToPlay = newlyPressedButton;
        Serial.print("New Press: Button "); Serial.println(buttonToPlay);
    } else if (currentButtonReleased) {
        // Current button released (could be 0-9 or L). 
        // Try buffer/fallback ONLY for buttons 0-9. L doesn't participate in retrigger.
        int lastHeldButtonInBuffer = -1;
        if (state.currentButton != BTN_L) { // Only check buffer if released button wasn't L
            int readIndex = state.lastPressedIndex;
            for (int i = 0; i < LAST_PRESS_BUFFER_SIZE; ++i) {
                readIndex = (readIndex + LAST_PRESS_BUFFER_SIZE - 1) % LAST_PRESS_BUFFER_SIZE;
                int bufferedButton = state.lastPressedBuffer[readIndex];
                // Check buffer only contains buttons 0-9, held, and not the one released
                if (bufferedButton >= 0 && bufferedButton < MAX_NOTE_BUTTONS &&
                    state.held[bufferedButton] && bufferedButton != state.currentButton) {
                    lastHeldButtonInBuffer = bufferedButton;
                    break;
                }
            }
        }
        
        if (lastHeldButtonInBuffer != -1) {
             buttonToPlay = lastHeldButtonInBuffer;
             Serial.print("Retrigger (Buffer): Button "); Serial.println(buttonToPlay);
        } else {
            // Buffer empty or didn't apply. Fallback to lowest held (0-9 only).
            int lowestFallbackHeld = -1;
            for (int btnIndex = 0; btnIndex < MAX_NOTE_BUTTONS; ++btnIndex) { // Check only 0-9
                 if (state.held[btnIndex] && btnIndex != state.currentButton) {
                     lowestFallbackHeld = btnIndex;
                     break;
                 }
            }
            if (lowestFallbackHeld != -1) {
                 buttonToPlay = lowestFallbackHeld;
                 Serial.print("Retrigger (Fallback): Button "); Serial.println(buttonToPlay);
            } else {
                 // Fallback (0-9) failed. Check if L is held (Thunderstruck only).
                 if (state.customProfileIndex == PROFILE_THUNDERSTRUCK && state.held[BTN_L]) {
                     buttonToPlay = BTN_L; // Retrigger L if it's still held
                     Serial.print("Retrigger (L Held): Button "); Serial.println(buttonToPlay);
                 } else {
                     // Nothing else held (0-9 or L), stop the note.
                     stopCurrentNote = true;
                     Serial.println("Stop Note: Button released, nothing else held");
                 }
            }
        }
    } else if (state.customProfileIndex == PROFILE_THUNDERSTRUCK && state.pressed[BTN_L]) {
        // Special case: L pressed *by itself* (no 0-9 button pressed) in Thunderstruck profile
        buttonToPlay = BTN_L; // Assign BTN_L index (10)
        Serial.print("New Press: L Button (Thunderstruck) -> "); Serial.println(buttonToPlay);
    } else if (pitchBendChanged) {
        // Pitch bend changed while a note was held - retrigger the same button (could be 0-9 or L)
        buttonToPlay = state.currentButton;
        Serial.print("Retrigger (Pitch Bend): Button "); Serial.println(buttonToPlay);
    } else {
         // No relevant change
         buttonToPlay = -1;
    }

    // --- Execute Action ---
    // Stop Note if necessary
    if (stopCurrentNote && state.currentMidiNote != -1) {
        sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
        stopNote(0); // Use voice 0 for mono
        state.currentMidiNote = -1;
        state.currentFrequency = 0.0;
        state.currentButton = -1;
    }
    // Play Note if necessary
    else if (buttonToPlay != -1)
    {
        int baseMidiNote; // Note before pitch bend

        // *** PROFILE LOGIC ***
        if (state.customProfileIndex == PROFILE_THUNDERSTRUCK) {
            if (buttonToPlay == BTN_L) {
                baseMidiNote = 71; // L button itself plays Open B
                DEBUG_DEBUG(CAT_PLAYSTYLE, "Thunderstruck profile: L Button -> Base MIDI %d", baseMidiNote);
            } else if (buttonToPlay >= 0 && buttonToPlay < MAX_NOTE_BUTTONS) { // Check button index is valid for array (0-9)
                baseMidiNote = thunderstruckMidiNotes[buttonToPlay]; // Use mapping for buttons 0-9
                DEBUG_DEBUG(CAT_PLAYSTYLE, "Thunderstruck profile: Button %d -> Base MIDI %d", buttonToPlay, baseMidiNote);
            } else {
                // Should not happen if buttonToPlay is set correctly (e.g., R button isn't assigned)
                baseMidiNote = 0; 
                DEBUG_WARNING(CAT_PLAYSTYLE, "Invalid buttonToPlay index in Thunderstruck profile: %d", buttonToPlay);
            }
        } else {
            // Standard Scale Profile (uses buttons 0-9)
            if (buttonToPlay >= 0 && buttonToPlay < MAX_NOTE_BUTTONS) {
                int musicalPosition = buttonToMusicalPosition[buttonToPlay];
                baseMidiNote = state.scaleHolder[musicalPosition];
                DEBUG_DEBUG(CAT_PLAYSTYLE, "Scale profile: Button %d -> Pos %d -> Base MIDI %d", buttonToPlay, musicalPosition, baseMidiNote);
            } else {
                 baseMidiNote = 0;
                 DEBUG_WARNING(CAT_PLAYSTYLE, "Invalid buttonToPlay index in Scale profile: %d", buttonToPlay);
            }
        }

        // Apply Pitch Bend (which is profile-dependent now)
        int midiNote = baseMidiNote + pitchBendOffset;
        DEBUG_DEBUG(CAT_PLAYSTYLE, "  -> Applied PB %d -> Final MIDI %d", pitchBendOffset, midiNote);

        // Clamp MIDI note
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