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

// --- Variable Duration 8th Note Boogie Mode --- V12.7 (L+R Triplets)
void handleBoogieTiming(SynthState& state) {
    // --- Basic Setup & Tempo Check --- 
    if (!state.tempoEstablished || state.usPerMidiTick <= 0) { 
        if (state.boogieCurrentMidiNote != -1) { /* Stop Note & Reset */
            DEBUG_INFO(CAT_PLAYSTYLE, "Boogie V12.7 Stop: Tempo not established/invalid.");
            sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL); stopNote(0);
            state.boogieCurrentMidiNote = -1; state.boogieTriggerButton = -1; state.boogieCurrentSlotIndex = -1; state.boogieNoteStopTimeMicros = 0; state.boogieInternalBeatStartTimeMicros = 0;
        }
        return;
    }
    unsigned long nowMicros = micros();

    // --- Handle Clock State Transitions --- (Same as V12.5)
    bool clockJustStopped = !state.midiSyncEnabled && state.prevMidiSyncEnabled;
    bool clockJustStarted = state.midiSyncEnabled && !state.prevMidiSyncEnabled;

    if (clockJustStopped) {
        DEBUG_INFO(CAT_PLAYSTYLE, "Boogie V12.7: Clock stopped. Switching to Internal Trigger mode.");
        if (state.boogieCurrentMidiNote != -1) { sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL); stopNote(0); }
        state.boogieCurrentMidiNote = -1; state.boogieTriggerButton = -1; state.boogieCurrentSlotIndex = -1; state.boogieNoteStopTimeMicros = 0; 
    }
    if (clockJustStarted) {
         DEBUG_INFO(CAT_PLAYSTYLE, "Boogie V12.7: Clock started. Switching to External Sync mode.");
         if (state.boogieCurrentMidiNote != -1) { sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL); stopNote(0); }
         state.boogieCurrentMidiNote = -1; state.boogieTriggerButton = -1; state.boogieCurrentSlotIndex = -1; state.boogieNoteStopTimeMicros = 0;
    }

    // --- Calculate Core Timing --- 
    float quarterNoteDurationMicros = state.usPerMidiTick * 24.0f;
    if (quarterNoteDurationMicros <= 0) return; // Safety check

    // --- Get Input & Prioritized Note --- (Needed early for trigger logic)
    int newlyPressedButton = -1;
    for (int i = 0; i < MAX_NOTE_BUTTONS; ++i) { if (state.pressed[i]) { newlyPressedButton = i; break; } }
    int prioritizedBaseMidiNote = getBaseMidiNote(state);

    // --- Determine Beat Reference Time --- (Needed early for trigger logic)
    unsigned long currentBeatRefTimeMicros = 0;
    if (state.midiSyncEnabled) {
        currentBeatRefTimeMicros = state.beatStartTimeMicros;
    } else if (state.boogieTriggerButton != -1) {
        currentBeatRefTimeMicros = state.boogieInternalBeatStartTimeMicros;
    } // Else: 0 if inactive internal mode

    // --- Handle Sequence Stop/Start Trigger (Button Presses/Releases) --- (Logic is independent of rhythm type)
    if (state.midiSyncEnabled) {
        // External Sync Mode Trigger/Stop
        if (state.boogieTriggerButton != -1 && prioritizedBaseMidiNote == -1) { 
             DEBUG_INFO(CAT_PLAYSTYLE, "Boogie V12.7 Ext Stop Trigger: No Button Held");
             if (state.boogieCurrentMidiNote != -1) { sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL); stopNote(0); state.boogieCurrentMidiNote = -1; state.boogieCurrentSlotIndex = -1; }
             state.boogieTriggerButton = -1; 
        } else if (state.boogieTriggerButton == -1 && newlyPressedButton != -1 && prioritizedBaseMidiNote != -1) {
            DEBUG_INFO(CAT_PLAYSTYLE, "Boogie V12.7 Ext Start Trigger");
            state.boogieTriggerButton = newlyPressedButton; 
        }
    } else {
        // Internal Trigger Mode Trigger/Stop
         if (state.boogieTriggerButton != -1 && prioritizedBaseMidiNote == -1) {
             DEBUG_INFO(CAT_PLAYSTYLE, "Boogie V12.7 Int Stop Trigger: No Button Held");
             if (state.boogieCurrentMidiNote != -1) { sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL); stopNote(0); state.boogieCurrentMidiNote = -1; state.boogieCurrentSlotIndex = -1; }
             state.boogieTriggerButton = -1; 
             state.boogieInternalBeatStartTimeMicros = 0; 
        } else if (state.boogieTriggerButton == -1 && newlyPressedButton != -1 && prioritizedBaseMidiNote != -1) {
            DEBUG_INFO(CAT_PLAYSTYLE, "Boogie V12.7 Int Start Trigger @ %lu", nowMicros);
            state.boogieTriggerButton = newlyPressedButton; 
            state.boogieInternalBeatStartTimeMicros = nowMicros; 
            currentBeatRefTimeMicros = state.boogieInternalBeatStartTimeMicros; // Update local ref immediately
        }
    }

    // --- Check if sequence is active before proceeding to rhythm generation ---
    if (state.boogieTriggerButton == -1 || currentBeatRefTimeMicros == 0 || prioritizedBaseMidiNote == -1) {
         // If sequence isn't active, or beat ref is invalid, or no note button held, ensure silence if needed and exit
         if (state.boogieCurrentMidiNote != -1) { // Ensure note off if sequence just stopped
              DEBUG_VERBOSE(CAT_PLAYSTYLE, "Boogie V12.7 Stopping lingering note %d as sequence became inactive.", state.boogieCurrentMidiNote);
              sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL); stopNote(0);
              state.boogieCurrentMidiNote = -1; state.boogieCurrentSlotIndex = -1;
         }
         return; // Nothing more to do rhythmically
    }
    
    // === RHYTHM GENERATION === 
    unsigned long elapsedInCurrentBeat = nowMicros - currentBeatRefTimeMicros;
    elapsedInCurrentBeat = elapsedInCurrentBeat % (unsigned long)quarterNoteDurationMicros; // Rollover
    int targetSlot = -1; // Which slot (0,1 for 8ths; 0,1,2 for triplets) should play?

    // --- Select Mode: Triplets (L+R) or Swing 8ths --- 
    if (state.held[BTN_L] && state.held[BTN_R]) {
        // --- TRIPLET MODE --- 
        DEBUG_DEBUG(CAT_PLAYSTYLE, "[Triplet Mode Active]");
        float tripletDurationMicros = quarterNoteDurationMicros / 3.0f;
        unsigned long tripletNoteDuration = (unsigned long)(tripletDurationMicros * 0.5f); // 50% duration

        // Determine current triplet slot
        int currentTripletSlot = (int)(elapsedInCurrentBeat / tripletDurationMicros); 
        if (currentTripletSlot < 0) currentTripletSlot = 0; if (currentTripletSlot > 2) currentTripletSlot = 2;

        unsigned long tripletAbsStartTime = currentBeatRefTimeMicros + (unsigned long)(currentTripletSlot * tripletDurationMicros); 
        unsigned long tripletAbsStopTime = tripletAbsStartTime + tripletNoteDuration;

        // 1a. Handle Triplet Note Off (Immediate stop if needed)
        if (state.boogieCurrentMidiNote != -1) {
            DEBUG_DEBUG(CAT_PLAYSTYLE, "[Triplet OFF Check] Note %d playing in Slot %d", state.boogieCurrentMidiNote, state.boogieCurrentSlotIndex); // Add Log 1
            if (state.boogieCurrentSlotIndex >= 0 && state.boogieCurrentSlotIndex <= 2) { // Simple check if it was a triplet note
                 float playingTripletDuration = quarterNoteDurationMicros / 3.0f;
                 unsigned long playingNoteDuration = (unsigned long)(playingTripletDuration * 0.5f);
                 unsigned long playingTripletAbsStartTime = state.boogieNoteStartTimeMicros; // Time it actually started
                 unsigned long playingTripletAbsStopTime = playingTripletAbsStartTime + playingNoteDuration; // When it should stop
                 DEBUG_DEBUG(CAT_PLAYSTYLE, "  [Triplet OFF Check] Triplet note stop check: Now:%lu, StopTime:%lu", nowMicros, playingTripletAbsStopTime); // Add Log 2
                 if (nowMicros >= playingTripletAbsStopTime) {
                     DEBUG_VERBOSE(CAT_PLAYSTYLE, "Boogie Triplet Note Stop: Slot %d, Note %d", state.boogieCurrentSlotIndex, state.boogieCurrentMidiNote);
                     sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL);
                     stopNote(0);
                     state.boogieCurrentMidiNote = -1; // Reset note
                     state.boogieCurrentSlotIndex = -1; 
                     DEBUG_DEBUG(CAT_PLAYSTYLE, "  [Triplet OFF Check] Note Stopped. boogieCurrentMidiNote = %d", state.boogieCurrentMidiNote); // Add Log 3
                 }
             } else { // Note was from 8th note mode, stop it immediately
                  DEBUG_VERBOSE(CAT_PLAYSTYLE, "Boogie Triplet Mode: Stopping lingering 8th note %d", state.boogieCurrentMidiNote);
                  sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL); stopNote(0);
                  state.boogieCurrentMidiNote = -1; // Reset note
                  state.boogieCurrentSlotIndex = -1; 
                  DEBUG_DEBUG(CAT_PLAYSTYLE, "  [Triplet OFF Check] Lingering 8th Note Stopped. boogieCurrentMidiNote = %d", state.boogieCurrentMidiNote); // Add Log 4
             }
        }

        // 3a. Handle Triplet Note On 
        if (state.boogieCurrentMidiNote == -1) { // Only if silent
            // Check if now is within the current triplet slot's play window
            DEBUG_DEBUG(CAT_PLAYSTYLE, "[Triplet ON Check] Now:%lu, Slot:%d, Start:%lu, Stop:%lu", nowMicros, currentTripletSlot, tripletAbsStartTime, tripletAbsStopTime);
            if (nowMicros >= tripletAbsStartTime && nowMicros < tripletAbsStopTime) {
                targetSlot = currentTripletSlot; // Mark that we want to play this triplet slot
            }
        }

    } else {
        // --- SWING 8TH NOTE MODE --- 
        float eighthNoteNominalDuration = quarterNoteDurationMicros / 2.0f;
        float swingDelayMicros = state.swingAmount * (quarterNoteDurationMicros / 6.0f);
        unsigned long slot0StartTimeRel = 0; 
        unsigned long slot1StartTimeRel = (unsigned long)(eighthNoteNominalDuration + swingDelayMicros);
        unsigned long note0IntendedDuration = (unsigned long)(eighthNoteNominalDuration * 0.5f); 
        unsigned long note1IntendedDuration = (unsigned long)(eighthNoteNominalDuration * 0.5f); 
        unsigned long slot0StopTimeRel = min(note0IntendedDuration, slot1StartTimeRel); 
        unsigned long slot1StopTimeRel = min(slot1StartTimeRel + note1IntendedDuration, (unsigned long)quarterNoteDurationMicros);

        // 1b. Handle Immediate Mute Press Stops
        if (state.boogieCurrentMidiNote != -1) { 
            if (state.pressed[BTN_L] && state.boogieCurrentSlotIndex == 0) {
                DEBUG_VERBOSE(CAT_PLAYSTYLE, "Boogie Mute Stop: L pressed, stopping Slot 0 note %d", state.boogieCurrentMidiNote);
                sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL); stopNote(0);
                state.boogieCurrentMidiNote = -1; state.boogieCurrentSlotIndex = -1; 
            } else if (state.pressed[BTN_R] && state.boogieCurrentSlotIndex == 1) {
                DEBUG_VERBOSE(CAT_PLAYSTYLE, "Boogie Mute Stop: R pressed, stopping Slot 1 note %d", state.boogieCurrentMidiNote);
                sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL); stopNote(0);
                state.boogieCurrentMidiNote = -1; state.boogieCurrentSlotIndex = -1; 
            }
        }

        // 2b. Handle Scheduled Note Off (Only if not already stopped by mute)
        if (state.boogieCurrentMidiNote != -1) { 
             // Need absolute stop time relative to the beat the note started on
             unsigned long beatNumPlaying = (state.boogieNoteStartTimeMicros - currentBeatRefTimeMicros) / (unsigned long)quarterNoteDurationMicros; 
             unsigned long currentNoteAbsStopTime = currentBeatRefTimeMicros + (beatNumPlaying * (unsigned long)quarterNoteDurationMicros) + 
                                                  ((state.boogieCurrentSlotIndex == 0) ? slot0StopTimeRel : slot1StopTimeRel);

            if (nowMicros >= currentNoteAbsStopTime) {
                DEBUG_VERBOSE(CAT_PLAYSTYLE, "Boogie 8th Note Stop: Slot %d, Note %d", state.boogieCurrentSlotIndex, state.boogieCurrentMidiNote);
                sendMidiNoteOff(state.boogieCurrentMidiNote, 0, MIDI_CHANNEL);
                stopNote(0);
                state.boogieCurrentMidiNote = -1;
                state.boogieCurrentSlotIndex = -1; 
            }
        }

        // 3b. Handle 8th Note On 
        if (state.boogieCurrentMidiNote == -1) { // Only if silent
            bool muteSlot0 = state.held[BTN_L];
            bool muteSlot1 = state.held[BTN_R];
            unsigned long slot0AbsStart = currentBeatRefTimeMicros + slot0StartTimeRel;
            unsigned long slot0AbsStop = currentBeatRefTimeMicros + slot0StopTimeRel;
            unsigned long slot1AbsStart = currentBeatRefTimeMicros + slot1StartTimeRel;
            unsigned long slot1AbsStop = currentBeatRefTimeMicros + slot1StopTimeRel;
            
            // Adjust for current beat rollover if beatRefTime is old
             unsigned long beatNumCurrent = (nowMicros - currentBeatRefTimeMicros) / (unsigned long)quarterNoteDurationMicros;
             slot0AbsStart += (beatNumCurrent * (unsigned long)quarterNoteDurationMicros);
             slot0AbsStop += (beatNumCurrent * (unsigned long)quarterNoteDurationMicros);
             slot1AbsStart += (beatNumCurrent * (unsigned long)quarterNoteDurationMicros);
             slot1AbsStop += (beatNumCurrent * (unsigned long)quarterNoteDurationMicros);

            // Check Slot 0
            if (!muteSlot0 && nowMicros >= slot0AbsStart && nowMicros < slot0AbsStop) {
                 targetSlot = 0;
            }
            // Check Slot 1
            if (targetSlot == -1 && !muteSlot1 && nowMicros >= slot1AbsStart && nowMicros < slot1AbsStop) {
                 targetSlot = 1;
            }
        }
    } // End of Swing 8th Note Mode


    // --- Play Note if Target Slot Determined --- 
    if (targetSlot != -1) {
        int targetNote = prioritizedBaseMidiNote - 24; if (targetNote < 0) targetNote = 0; if (targetNote > 127) targetNote = 127;
        unsigned long targetAbsStopTime; // Recalculate based on target slot for clarity
        if (state.held[BTN_L] && state.held[BTN_R]) { // Triplet timings
            float tripletDurationMicros = quarterNoteDurationMicros / 3.0f;
            unsigned long tripletNoteDuration = (unsigned long)(tripletDurationMicros * 0.5f);
             unsigned long beatNumCurrent = (nowMicros - currentBeatRefTimeMicros) / (unsigned long)quarterNoteDurationMicros;
            unsigned long targetAbsStartTime = currentBeatRefTimeMicros + (beatNumCurrent * (unsigned long)quarterNoteDurationMicros) + (unsigned long)(targetSlot * tripletDurationMicros);
             targetAbsStopTime = targetAbsStartTime + tripletNoteDuration;
             DEBUG_VERBOSE(CAT_PLAYSTYLE, "Boogie Triplet Note Start: Slot %d, Note %d, Stop @ %lu", targetSlot, targetNote, targetAbsStopTime);
              state.boogieNoteStartTimeMicros = targetAbsStartTime; // Store actual start time
        } else { // 8th note timings
             float eighthNoteNominalDuration = quarterNoteDurationMicros / 2.0f;
             float swingDelayMicros = state.swingAmount * (quarterNoteDurationMicros / 6.0f);
             unsigned long slot0StartTimeRel = 0; 
             unsigned long slot1StartTimeRel = (unsigned long)(eighthNoteNominalDuration + swingDelayMicros);
             unsigned long note0IntendedDuration = (unsigned long)(eighthNoteNominalDuration * 0.5f); 
             unsigned long note1IntendedDuration = (unsigned long)(eighthNoteNominalDuration * 0.5f); 
             unsigned long slot0StopTimeRel = min(note0IntendedDuration, slot1StartTimeRel); 
             unsigned long slot1StopTimeRel = min(slot1StartTimeRel + note1IntendedDuration, (unsigned long)quarterNoteDurationMicros);
             unsigned long beatNumCurrent = (nowMicros - currentBeatRefTimeMicros) / (unsigned long)quarterNoteDurationMicros;
             unsigned long targetAbsStartTime = currentBeatRefTimeMicros + (beatNumCurrent * (unsigned long)quarterNoteDurationMicros) + 
                                             ((targetSlot == 0) ? slot0StartTimeRel : slot1StartTimeRel);
             targetAbsStopTime = currentBeatRefTimeMicros + (beatNumCurrent * (unsigned long)quarterNoteDurationMicros) + 
                                             ((targetSlot == 0) ? slot0StopTimeRel : slot1StopTimeRel);
            DEBUG_VERBOSE(CAT_PLAYSTYLE, "Boogie 8th Note Start: Slot %d, Note %d, Stop @ %lu", targetSlot, targetNote, targetAbsStopTime);
             state.boogieNoteStartTimeMicros = targetAbsStartTime; // Store actual start time
        }

        playNote(state, 0, targetNote);
        sendMidiNoteOn(targetNote, MIDI_VELOCITY, MIDI_CHANNEL);
        state.boogieCurrentMidiNote = targetNote;
        state.boogieCurrentSlotIndex = targetSlot; // Use targetSlot (0,1, or 2)
        state.boogieNoteStopTimeMicros = targetAbsStopTime; // Store calculated stop time
    }
}

// Monophonic playstyle - V3 Revert + Fixes
void handleMonophonic(SynthState& state) {
    DEBUG_DEBUG(CAT_PLAYSTYLE, "--- Entered handleMonophonic ---"); // ADD DEBUG
    // --- Determine Inputs ---
    int newlyPressedButton = -1;
    int releasedButton = -1;
    int highestPriorityHeldButton = -1; // Track the button we *should* be playing if held

    for (int i = 0; i < MAX_NOTE_BUTTONS; ++i) {
        if (state.pressed[i]) {
            newlyPressedButton = i;
            // Don't break here, need to check all for highestPriorityHeldButton
        }
        // Check if the *currently playing* button was released
        if (state.released[i] && i == state.currentButton) { 
             releasedButton = i;
        }
        // Find lowest index held button
        if (state.held[i] && highestPriorityHeldButton == -1) { 
            highestPriorityHeldButton = i;
        }
    }
    // Special check for L press in Thunderstruck
    if (newlyPressedButton == -1 && state.customProfileIndex == PROFILE_THUNDERSTRUCK && state.pressed[BTN_L]) {
        newlyPressedButton = BTN_L;
        // L can be the "held" button in TS if nothing else is held
        if (highestPriorityHeldButton == -1) highestPriorityHeldButton = BTN_L; 
    }


    // --- Determine Current Pitch Bend ---
    int currentPitchBend = 0;
    if (state.customProfileIndex != PROFILE_THUNDERSTRUCK) {
        if (state.held[BTN_L] && state.held[BTN_R]) currentPitchBend = 0;
        else if (state.held[BTN_L]) currentPitchBend = -12;
        else if (state.held[BTN_R]) currentPitchBend = 12;
    } else { 
         if (state.held[BTN_R]) currentPitchBend = 12;
    }
    bool pitchBendChanged = (currentPitchBend != state.prevPitchBend);

    // --- Logic ---

    // 1. Handle New Button Press (Highest Priority)
    if (newlyPressedButton != -1) {
        // Send Note Off for the previous note if it was different
        if (state.currentMidiNote != -1 && state.currentButton != newlyPressedButton) {
             DEBUG_VERBOSE(CAT_MIDI, "Mono MIDI Note Off (Before New Press): %d", state.currentMidiNote);
             sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
             // Don't stop audio, playNote handles retrigger/portamento
        }

        // Get Base Note for the newly pressed button
        int baseMidiNote = -1;
         if (state.customProfileIndex == PROFILE_THUNDERSTRUCK) {
            if (newlyPressedButton == BTN_L) baseMidiNote = 71;
            else if (newlyPressedButton < MAX_NOTE_BUTTONS) baseMidiNote = thunderstruckMidiNotes[newlyPressedButton];
         } else { // Standard Scale Profile
             if (newlyPressedButton < MAX_NOTE_BUTTONS) {
                int musicalPosition = buttonToMusicalPosition[newlyPressedButton];
                if (musicalPosition >= 0 && musicalPosition < 12) baseMidiNote = state.scaleHolder[musicalPosition];
                else { DEBUG_WARNING(CAT_PLAYSTYLE, "Mono Press: Invalid musicalPosition %d for button %d", musicalPosition, newlyPressedButton); }
             }
         }

         if (baseMidiNote != -1) {
             int finalMidiNote = baseMidiNote + currentPitchBend;
             if (finalMidiNote < 0) finalMidiNote = 0; if (finalMidiNote > 127) finalMidiNote = 127;

             DEBUG_INFO(CAT_PLAYSTYLE, "Mono Press: base=%d, bend=%d, final=%d (Button %d)", baseMidiNote, currentPitchBend, finalMidiNote, newlyPressedButton);
             playNote(state, 0, finalMidiNote);
             sendMidiNoteOn(finalMidiNote, MIDI_VELOCITY, MIDI_CHANNEL);

             state.currentMidiNote = finalMidiNote;
             state.currentButton = newlyPressedButton; // Update the currently playing button
             state.currentFrequency = midiToPitchFloat[finalMidiNote];
         } else {
              DEBUG_WARNING(CAT_PLAYSTYLE, "Mono Press: Could not get note for button %d", newlyPressedButton);
              // If press failed to get note, ensure previous note is stopped?
              if(state.currentMidiNote != -1) sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
              stopNote(0); state.currentMidiNote = -1; state.currentButton = -1; state.currentFrequency = 0.0;
         }
    }
    // 2. Handle Release of the Playing Button (If no new button was pressed)
    else if (releasedButton != -1) {
        // --- Retrigger Check ---
        // Find the highest priority button *still held*
        int buttonToRetrigger = -1;
         for (int i = 0; i < MAX_NOTE_BUTTONS; ++i) { 
             // Check state.held[] directly for what is held NOW
             if (i != releasedButton && state.held[i]) { 
                 buttonToRetrigger = i;
                 break;
             }
         }
         // Retriggering L in Thunderstruck is handled by its own press logic if needed.

        if (buttonToRetrigger != -1) {
            // Retrigger the highest priority *remaining* held button
             DEBUG_INFO(CAT_PLAYSTYLE, "Mono Retrigger: Button %d released, retriggering held button %d", releasedButton, buttonToRetrigger);
             
             // Send Note Off for the *released* note
             if (state.currentMidiNote != -1) {
                  sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
             }

             // Get Base Note for the button to retrigger
             int baseMidiNote = -1;
             if (state.customProfileIndex == PROFILE_THUNDERSTRUCK) {
                  if (buttonToRetrigger < MAX_NOTE_BUTTONS) baseMidiNote = thunderstruckMidiNotes[buttonToRetrigger];
             } else { // Standard Scale Profile
                  if (buttonToRetrigger < MAX_NOTE_BUTTONS) {
                      int musicalPosition = buttonToMusicalPosition[buttonToRetrigger];
                       if (musicalPosition >= 0 && musicalPosition < 12) baseMidiNote = state.scaleHolder[musicalPosition];
                       else { DEBUG_WARNING(CAT_PLAYSTYLE, "Mono Retrigger: Invalid musicalPosition %d for button %d", musicalPosition, buttonToRetrigger); }
                  }
             }

              if (baseMidiNote != -1) {
                 int finalMidiNote = baseMidiNote + currentPitchBend; // Use current bend
                 if (finalMidiNote < 0) finalMidiNote = 0; if (finalMidiNote > 127) finalMidiNote = 127;

                 DEBUG_INFO(CAT_PLAYSTYLE, "Mono Retrigger Play: base=%d, bend=%d, final=%d (Button %d)", baseMidiNote, currentPitchBend, finalMidiNote, buttonToRetrigger);
                 playNote(state, 0, finalMidiNote);
                 sendMidiNoteOn(finalMidiNote, MIDI_VELOCITY, MIDI_CHANNEL);

                 state.currentMidiNote = finalMidiNote;
                 state.currentButton = buttonToRetrigger; // Update to the retriggered button
                 state.currentFrequency = midiToPitchFloat[finalMidiNote];
             } else {
                  DEBUG_WARNING(CAT_PLAYSTYLE, "Mono Retrigger: Could not get note for button %d", buttonToRetrigger);
                  stopNote(0); state.currentMidiNote = -1; state.currentButton = -1; state.currentFrequency = 0.0;
             }
        } else {
            // Stop Note (Nothing else held)
             DEBUG_DEBUG(CAT_PLAYSTYLE, "[Mono Stop]: Entering 'Stop Note (Nothing else held)' block for released button %d.", releasedButton); // Added log
             if (state.currentMidiNote != -1) {
                int noteToStop = state.currentMidiNote; // Store note before resetting state
                DEBUG_DEBUG(CAT_MIDI, "[Mono Stop]: Attempting MIDI Note Off for %d", noteToStop); // Added log
                sendMidiNoteOff(noteToStop, 0, MIDI_CHANNEL);
                DEBUG_DEBUG(CAT_MIDI, "[Mono Stop]: MIDI Note Off Sent for %d.", noteToStop); // Added log
             }
            DEBUG_DEBUG(CAT_AUDIO, "[Mono Stop]: Attempting stopNote(0)."); // Added log
            stopNote(0);
            DEBUG_DEBUG(CAT_AUDIO, "[Mono Stop]: stopNote(0) called."); // Added log
            state.currentMidiNote = -1; state.currentButton = -1; state.currentFrequency = 0.0;
        }
    }
    // 3. Handle Pitch Bend Change Only (If no press or release of playing note occurred)
    else if (pitchBendChanged && state.currentButton != -1) {
        // Get Base Note for the *currently* playing button (check state.currentButton)
        int baseMidiNote = -1;
        if (state.customProfileIndex == PROFILE_THUNDERSTRUCK) {
            if (state.currentButton == BTN_L) baseMidiNote = 71;
            else if (state.currentButton < MAX_NOTE_BUTTONS) baseMidiNote = thunderstruckMidiNotes[state.currentButton];
         } else { 
             if (state.currentButton < MAX_NOTE_BUTTONS) {
                int musicalPosition = buttonToMusicalPosition[state.currentButton];
                if (musicalPosition >= 0 && musicalPosition < 12) baseMidiNote = state.scaleHolder[musicalPosition];
                 else { DEBUG_WARNING(CAT_PLAYSTYLE, "Mono Bend Change: Invalid musicalPosition %d for button %d", musicalPosition, state.currentButton); }
             }
         }

         if (baseMidiNote != -1) {
             int finalMidiNote = baseMidiNote + currentPitchBend; // Apply NEW bend
             if (finalMidiNote < 0) finalMidiNote = 0; if (finalMidiNote > 127) finalMidiNote = 127;

             DEBUG_INFO(CAT_PLAYSTYLE, "Mono Bend Change: base=%d, bend=%d, final=%d (Button %d)", baseMidiNote, currentPitchBend, finalMidiNote, state.currentButton);
             
             // Send Note Off for previous pitch ONLY if note number changes
              if (state.currentMidiNote != -1 && state.currentMidiNote != finalMidiNote) {
                  sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
              }
             playNote(state, 0, finalMidiNote); // Retrigger audio with new pitch
             
             // Send Note On only if note number changed or was previously off
              if (state.currentMidiNote == -1 || state.currentMidiNote != finalMidiNote) {
                 sendMidiNoteOn(finalMidiNote, MIDI_VELOCITY, MIDI_CHANNEL);
              }

             // Update state (only note, not button)
             state.currentMidiNote = finalMidiNote;
             state.currentFrequency = midiToPitchFloat[finalMidiNote];
         } else {
              DEBUG_WARNING(CAT_PLAYSTYLE, "Mono Bend Change: Could not get note for current button %d", state.currentButton);
         }
    }

    // Update previous pitch bend state for next cycle comparison
    state.prevPitchBend = currentPitchBend;
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
            // Serial.print("Retriggering chord (Buffer Hit) for button: "); Serial.println(lastHeldButtonInBuffer); // Commented out
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
                 // Serial.print("Retriggering chord (Fallback Held - Lowest) for button: "); Serial.println(lowestFallbackHeld); // Commented out
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
             // Serial.println("Stopping chord (button released w/o retrigger or none held)"); // Commented out
            bool notesWerePlaying = false;
            for (int i = 0; i < 4; i++) {
                if (state.currentChordNotes[i] != -1) {
                    notesWerePlaying = true;
                    stopNote(i);
                    DEBUG_VERBOSE(CAT_MIDI, "Chord MIDI Note Off (Stopping Chord): %d", state.currentChordNotes[i]);
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
            // Serial.println("Preparing voices for new/changed chord..."); // Commented out
             for (int i = 0; i < 4; i++) {
                if (state.currentChordNotes[i] != -1) {
                     DEBUG_VERBOSE(CAT_MIDI, "Chord MIDI Note Off (Prep New Chord): %d", state.currentChordNotes[i]);
                     sendMidiNoteOff(state.currentChordNotes[i], 0, MIDI_CHANNEL); 
                     
                     // *** CORRECTED STOP LOGIC ***
                     // Only stop audio voice if Portamento is OFF.
                     if (!state.portamentoEnabled) {
                         // Serial.println("   Stopping voice (Porta OFF)"); // Commented out
                         stopNote(i);
                         state.currentChordNotes[i] = -1; 
                         state.currentChordFrequencies[i] = 0.0;
                         state.waveformOpen[i] = 1;
                     } else {
                         // Serial.println("   Porta ON: Keeping voice active for slide."); // Commented out
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

        // Serial.print("Playing new chord for button "); Serial.print(state.currentButton); Serial.print(" (musical pos "); Serial.print(musicalPosition); Serial.println(")"); // Commented out
        
        // Play each note in the new chord
        for (int i = 0; i < numNotes; i++) {
            if (chordNotes[i] != -1) {
                int finalMidiNote = chordNotes[i] + newPitchBend; 
                if (finalMidiNote < 0) finalMidiNote = 0;
                if (finalMidiNote > 127) finalMidiNote = 127;
                
                // Serial.print("  Voice "); Serial.print(i); Serial.print(": MIDI "); Serial.println(finalMidiNote); // Commented out
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
                   // Serial.print("  Stopping unused voice "); Serial.println(i); // Commented out
                   stopNote(i);
                   DEBUG_VERBOSE(CAT_MIDI, "Chord MIDI Note Off (Unused Voice): %d", state.currentChordNotes[i]);
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