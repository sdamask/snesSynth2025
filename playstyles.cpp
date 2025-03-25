// playstyles.cpp
// Implements playstyle functions for the SNES synthesizer, handling different modes
// of note playback (Monophonic, ChordButton, Polyphonic).

#include "playstyles.h"
#include "synth.h"
#include "chords.h"
#include "audio.h"
#include "midi_utils.h"
#include <Arduino.h>

// Portamento settings
const int PORTAMENTO_STEPS = 90;  // Number of steps for portamento
const int PORTAMENTO_DELAY_MS = 6;  // Delay per step in milliseconds (90 * 6 = 540ms total glide time)

// Monophonic playstyle
void handleMonophonicPlayStyle(SynthState& state) {
    // Update pitchBend based on L (button 10) and R (button 11) states
    int currentLState = state.held[10];  // L button (octave down)
    int currentRState = state.held[11];  // R button (octave up)

    // Set pitchBend based on the current state of L and R
    if (currentLState == 1 && currentRState == 1) {
        // Both L and R are held: cancel out
        state.pitchBend = 0;
    } else if (currentLState == 1) {
        // Only L is held: octave down
        state.pitchBend = -12;
    } else if (currentRState == 1) {
        // Only R is held: octave up
        state.pitchBend = 12;
    } else {
        // Neither L nor R is held: neutral
        state.pitchBend = 0;
    }

    // Find the most recently pressed button (highest orderHeld value)
    int mostRecentButton = -1;
    int highestOrder = 0;
    for (int i = 0; i <= 9; i++) {  // Exclude L and R (buttons 10 and 11)
        if (state.orderHeld[i] > highestOrder) {
            highestOrder = state.orderHeld[i];
            mostRecentButton = i;
        }
    }

    // If no button is held, stop the current note
    if (mostRecentButton == -1) {
        if (state.currentButton != -1) {
            Serial.println("Stopping note (no buttons held)");
            stopNote(0);  // Use voice 0 for monophonic mode
            // Send MIDI Note Off
            if (state.currentMidiNote != -1) {
                sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
            }
            state.currentButton = -1;
            state.currentMidiNote = -1;
            state.currentFrequency = 0.0;
            state.waveformOpen[0] = 1;
        }
    } else if (mostRecentButton != state.currentButton) {
        // A new button was pressed, play the new note
        // Compute the MIDI note for the new button, including pitchBend
        int midiNote = state.scaleHolder[mostRecentButton] + state.pitchBend;
        // Ensure the MIDI note is within valid range (0-127)
        if (midiNote < 0) midiNote = 0;
        if (midiNote > 127) midiNote = 127;

        if (state.currentButton != -1) {
            Serial.println("Switching to new note");
            // Send MIDI Note Off for the previous note
            if (state.currentMidiNote != -1) {
                sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
                delay(5);
            }
            stopNote(0);  // Use voice 0 for monophonic mode
        }

        // Send MIDI Note On (let the synth handle portamento) - do this before the portamento slide
        sendMidiNoteOn(midiNote, MIDI_VELOCITY, MIDI_CHANNEL);
        state.waveformOpen[0] = 0;

        if (state.currentButton != -1 && state.portamentoEnabled) {
            // Portamento: Slide the pitch smoothly without stopping the note
            float startFreq = state.currentFrequency;
            float endFreq = midiToPitchFloat[midiNote];
            Serial.print("Starting portamento from ");
            Serial.print(startFreq);
            Serial.print(" Hz to ");
            Serial.print(endFreq);
            Serial.println(" Hz");

            // Ensure the envelope is active before starting portamento
            playNote(state, 0, midiNote);  // Re-trigger the note to ensure the envelope is active

            for (int i = 0; i <= PORTAMENTO_STEPS; i++) {
                float freq = startFreq + sin(i * state.degToRad) * (endFreq - startFreq);
                waveformMod[0].frequency(freq);  // Use voice 0 for monophonic mode
                delay(PORTAMENTO_DELAY_MS);  // Adjusted delay for slower glide
            }
            state.currentFrequency = endFreq;
            state.currentMidiNote = midiNote;
        } else {
            // Immediate change
            Serial.print("Playing new note: ");
            Serial.println(midiNote);
            playNote(state, 0, midiNote);  // Use voice 0 for monophonic mode
            state.currentFrequency = midiToPitchFloat[midiNote];
            state.currentMidiNote = midiNote;
        }

        // Update the current button
        state.currentButton = mostRecentButton;
    }

    // Check for releases and resume the previous note if necessary
    for (int i = 0; i <= 9; i++) {  // Exclude L and R
        if (state.released[i] == 1 && i == state.currentButton) {
            // The currently playing button was released
            // Find the next most recent button that's still held
            int nextButton = -1;
            int highestOrder = 0;
            for (int j = 0; j <= 9; j++) {  // Exclude L and R
                if (state.held[j] == 1 && state.orderHeld[j] > highestOrder) {
                    highestOrder = state.orderHeld[j];
                    nextButton = j;
                }
            }

            // If there's another button still held, play its note
            if (nextButton != -1) {
                // Compute the MIDI note for the next button, including pitchBend
                int midiNote = state.scaleHolder[nextButton] + state.pitchBend;
                if (midiNote < 0) midiNote = 0;
                if (midiNote > 127) midiNote = 127;

                // Send MIDI Note Off for the current note
                if (state.currentMidiNote != -1) {
                    sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
                    delay(5);
                }
                stopNote(0);  // Use voice 0 for monophonic mode

                // Send MIDI Note On (let the synth handle portamento) - do this before the portamento slide
                sendMidiNoteOn(midiNote, MIDI_VELOCITY, MIDI_CHANNEL);
                state.waveformOpen[0] = 0;

                if (state.portamentoEnabled) {
                    // Portamento: Slide the pitch smoothly without stopping the note
                    float startFreq = state.currentFrequency;
                    float endFreq = midiToPitchFloat[midiNote];
                    Serial.print("Starting portamento from ");
                    Serial.print(startFreq);
                    Serial.print(" Hz to ");
                    Serial.print(endFreq);
                    Serial.println(" Hz");

                    // Ensure the envelope is active before starting portamento
                    playNote(state, 0, midiNote);  // Re-trigger the note to ensure the envelope is active

                    for (int i = 0; i <= PORTAMENTO_STEPS; i++) {
                        float freq = startFreq + sin(i * state.degToRad) * (endFreq - startFreq);
                        waveformMod[0].frequency(freq);  // Use voice 0 for monophonic mode
                        delay(PORTAMENTO_DELAY_MS);  // Adjusted delay for slower glide
                    }
                    state.currentFrequency = endFreq;
                    state.currentMidiNote = midiNote;
                } else {
                    // Immediate change
                    Serial.println("Resuming previous note");
                    playNote(state, 0, midiNote);  // Use voice 0 for monophonic mode
                    state.currentFrequency = midiToPitchFloat[midiNote];
                    state.currentMidiNote = midiNote;
                }

                // Update the current button
                state.currentButton = nextButton;
            } else {
                // No other buttons are held, stop the note
                Serial.println("Stopping note (no other buttons held)");
                stopNote(0);  // Use voice 0 for monophonic mode
                if (state.currentMidiNote != -1) {
                    sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
                }
                state.currentButton = -1;
                state.currentMidiNote = -1;
                state.currentFrequency = 0.0;
                state.waveformOpen[0] = 1;
            }
        }
    }

    // Check for pitchBend changes while a note is playing
    static int prevPitchBend = 0;
    if (state.pitchBend != prevPitchBend && state.currentButton != -1) {
        int midiNote = state.scaleHolder[state.currentButton] + state.pitchBend;
        if (midiNote < 0) midiNote = 0;
        if (midiNote > 127) midiNote = 127;

        // Send MIDI Note Off for the current note
        if (state.currentMidiNote != -1) {
            sendMidiNoteOff(state.currentMidiNote, 0, MIDI_CHANNEL);
            delay(5);
        }
        stopNote(0);  // Use voice 0 for monophonic mode

        // Send MIDI Note On (let the synth handle portamento) - do this before the portamento slide
        sendMidiNoteOn(midiNote, MIDI_VELOCITY, MIDI_CHANNEL);
        state.waveformOpen[0] = 0;

        if (state.portamentoEnabled) {
            // Portamento: Slide the pitch smoothly without stopping the note
            float startFreq = state.currentFrequency;
            float endFreq = midiToPitchFloat[midiNote];
            Serial.print("Starting portamento from ");
            Serial.print(startFreq);
            Serial.print(" Hz to ");
            Serial.print(endFreq);
            Serial.println(" Hz");

            // Ensure the envelope is active before starting portamento
            playNote(state, 0, midiNote);  // Re-trigger the note to ensure the envelope is active

            for (int i = 0; i <= PORTAMENTO_STEPS; i++) {
                float freq = startFreq + sin(i * state.degToRad) * (endFreq - startFreq);
                waveformMod[0].frequency(freq);  // Use voice 0 for monophonic mode
                delay(PORTAMENTO_DELAY_MS);  // Adjusted delay for slower glide
            }
            state.currentFrequency = endFreq;
            state.currentMidiNote = midiNote;
        } else {
            // Immediate change
            stopNote(0);  // Use voice 0 for monophonic mode
            playNote(state, 0, midiNote);  // Use voice 0 for monophonic mode
            state.currentFrequency = midiToPitchFloat[midiNote];
            state.currentMidiNote = midiNote;
        }
    }
    prevPitchBend = state.pitchBend;
}

// ChordButton playstyle: Each button plays a chord (up to 4 notes)
void handleChordButtonPlayStyle(SynthState& state) {
    // Update pitchBend based on L (button 10) and R (button 11) states
    int currentLState = state.held[10];  // L button (octave down)
    int currentRState = state.held[11];  // R button (octave up)

    // Set pitchBend based on the current state of L and R
    if (currentLState == 1 && currentRState == 1) {
        state.pitchBend = 0;
    } else if (currentLState == 1) {
        state.pitchBend = -12;
    } else if (currentRState == 1) {
        state.pitchBend = 12;
    } else {
        state.pitchBend = 0;
    }

    // Find the most recently pressed button (highest orderHeld value)
    int mostRecentButton = -1;
    int highestOrder = 0;
    for (int i = 0; i <= 9; i++) {  // Exclude L and R (buttons 10 and 11)
        if (state.orderHeld[i] > highestOrder) {
            highestOrder = state.orderHeld[i];
            mostRecentButton = i;
        }
    }

    // If no button is held, stop the current chord
    if (mostRecentButton == -1) {
        if (state.currentButton != -1) {
            Serial.println("Stopping chord (no buttons held)");
            // Stop all notes in the current chord
            for (int i = 0; i < 4; i++) {
                if (state.currentChordNotes[i] != -1) {
                    Serial.print("Stopping voice ");
                    Serial.print(i);
                    Serial.print(": MIDI ");
                    Serial.println(state.currentChordNotes[i]);
                    sendMidiNoteOff(state.currentChordNotes[i], 0, MIDI_CHANNEL);
                    stopNote(i);
                    state.currentChordNotes[i] = -1;
                    state.currentChordFrequencies[i] = 0.0;  // Reset frequency
                    state.waveformOpen[i] = 1;
                }
            }
            state.currentButton = -1;
            state.currentFrequency = 0.0;
        }
    } else if (mostRecentButton != state.currentButton) {
        // A new button was pressed, play the new chord
        // Stop the previous chord (if any)
        if (state.currentButton != -1) {
            Serial.println("Switching to new chord");
            for (int i = 0; i < 4; i++) {
                if (state.currentChordNotes[i] != -1) {
                    Serial.print("Stopping voice ");
                    Serial.print(i);
                    Serial.print(": MIDI ");
                    Serial.println(state.currentChordNotes[i]);
                    sendMidiNoteOff(state.currentChordNotes[i], 0, MIDI_CHANNEL);
                    stopNote(i);
                    state.currentChordNotes[i] = -1;
                    state.waveformOpen[i] = 1;
                }
            }
        }

        // Get the chord notes for the new button
        int chordNotes[4];
        int numNotes;
        getChordNotes(state, mostRecentButton + 1, chordNotes, numNotes);  // 1-based scale degree

        // Play the new chord
        float startFreqs[4] = { 0 };
        float endFreqs[4] = { 0 };
        Serial.print("Playing chord for button ");
        Serial.print(mostRecentButton);
        Serial.print(": ");
        for (int i = 0; i < numNotes; i++) {
            int midiNote = chordNotes[i] + state.pitchBend;
            if (midiNote < 0) midiNote = 0;
            if (midiNote > 127) midiNote = 127;

            Serial.print("Voice ");
            Serial.print(i);
            Serial.print(": MIDI ");
            Serial.print(midiNote);
            if (i < numNotes - 1) Serial.print(", ");
            sendMidiNoteOn(midiNote, MIDI_VELOCITY, MIDI_CHANNEL);
            playNote(state, i, midiNote);  // Play the note immediately
            state.currentChordNotes[i] = midiNote;
            endFreqs[i] = midiToPitchFloat[midiNote];  // Store the target frequency
            startFreqs[i] = state.currentChordFrequencies[i];  // Use the actual frequency of the current note
            state.waveformOpen[i] = 0;
        }
        Serial.println();

        // Update the current button *before* portamento
        int previousButton = state.currentButton;
        state.currentButton = mostRecentButton;

        // Apply portamento to all notes in the chord
        if (previousButton != -1 && state.portamentoEnabled) {
            Serial.println("Starting portamento for chord");
            for (int i = 0; i < numNotes; i++) {
                Serial.print("Voice ");
                Serial.print(i);
                Serial.print(": ");
                Serial.print(startFreqs[i]);
                Serial.print(" Hz to ");
                Serial.print(endFreqs[i]);
                Serial.println(" Hz");
            }
            for (int i = 0; i <= PORTAMENTO_STEPS; i++) {
                float t = (float)i / PORTAMENTO_STEPS;  // Linear interpolation (0 to 1)
                for (int j = 0; j < numNotes; j++) {
                    float freq = startFreqs[j] + t * (endFreqs[j] - startFreqs[j]);  // Linear glide
                    waveformMod[j].frequency(freq);  // Use the correct voice for each note
                }
                delay(PORTAMENTO_DELAY_MS);  // Adjusted delay for slower glide
            }
        }

        // Update the frequencies after portamento (or immediate change)
        for (int i = 0; i < numNotes; i++) {
            state.currentChordFrequencies[i] = endFreqs[i];  // Store the actual frequency
        }
        state.currentFrequency = endFreqs[0];  // Use the root note’s frequency for portamento tracking
    }

    // Check for releases and resume the previous chord if necessary
    for (int i = 0; i <= 9; i++) {  // Exclude L and R
        if (state.released[i] == 1 && i == state.currentButton) {
            // The currently playing button was released
            // Find the next most recent button that's still held
            int nextButton = -1;
            int highestOrder = 0;
            for (int j = 0; j <= 9; j++) {  // Exclude L and R
                if (state.held[j] == 1 && state.orderHeld[j] > highestOrder) {
                    highestOrder = state.orderHeld[j];
                    nextButton = j;
                }
            }

            // If there's another button still held, play its chord
            if (nextButton != -1) {
                // Stop the current chord
                for (int j = 0; j < 4; j++) {
                    if (state.currentChordNotes[j] != -1) {
                        Serial.print("Stopping voice ");
                        Serial.print(j);
                        Serial.print(": MIDI ");
                        Serial.println(state.currentChordNotes[j]);
                        sendMidiNoteOff(state.currentChordNotes[j], 0, MIDI_CHANNEL);
                        stopNote(j);
                        state.currentChordNotes[j] = -1;
                        state.waveformOpen[j] = 1;
                    }
                }

                // Get the chord notes for the next button
                int chordNotes[4];
                int numNotes;
                getChordNotes(state, nextButton + 1, chordNotes, numNotes);  // 1-based scale degree

                // Play the new chord
                float startFreqs[4] = { 0 };
                float endFreqs[4] = { 0 };
                Serial.print("Playing chord for button ");
                Serial.print(nextButton);
                Serial.print(": ");
                for (int j = 0; j < numNotes; j++) {
                    int midiNote = chordNotes[j] + state.pitchBend;
                    if (midiNote < 0) midiNote = 0;
                    if (midiNote > 127) midiNote = 127;

                    Serial.print("Voice ");
                    Serial.print(j);
                    Serial.print(": MIDI ");
                    Serial.print(midiNote);
                    if (j < numNotes - 1) Serial.print(", ");
                    sendMidiNoteOn(midiNote, MIDI_VELOCITY, MIDI_CHANNEL);
                    playNote(state, j, midiNote);  // Play the note immediately
                    state.currentChordNotes[j] = midiNote;
                    endFreqs[j] = midiToPitchFloat[midiNote];  // Store the target frequency
                    startFreqs[j] = state.currentChordFrequencies[j];  // Use the actual frequency of the current note
                    state.waveformOpen[j] = 0;
                }
                Serial.println();

                // Update the current button *before* portamento
                int previousButton = state.currentButton;
                state.currentButton = nextButton;

                // Apply portamento to all notes in the chord
                if (previousButton != -1 && state.portamentoEnabled) {
                    Serial.println("Starting portamento for chord");
                    for (int i = 0; i < numNotes; i++) {
                        Serial.print("Voice ");
                        Serial.print(i);
                        Serial.print(": ");
                        Serial.print(startFreqs[i]);
                        Serial.print(" Hz to ");
                        Serial.print(endFreqs[i]);
                        Serial.println(" Hz");
                    }
                    for (int i = 0; i <= PORTAMENTO_STEPS; i++) {
                        float t = (float)i / PORTAMENTO_STEPS;  // Linear interpolation (0 to 1)
                        for (int j = 0; j < numNotes; j++) {
                            float freq = startFreqs[j] + t * (endFreqs[j] - startFreqs[j]);  // Linear glide
                            waveformMod[j].frequency(freq);  // Use the correct voice for each note
                        }
                        delay(PORTAMENTO_DELAY_MS);  // Adjusted delay for slower glide
                    }
                } else {
                    // Immediate change (no portamento)
                    for (int i = 0; i < numNotes; i++) {
                        playNote(state, i, state.currentChordNotes[i]);  // Play the note immediately
                    }
                }

                // Update the frequencies after portamento (or immediate change)
                for (int i = 0; i < numNotes; i++) {
                    state.currentChordFrequencies[i] = endFreqs[i];  // Store the actual frequency
                }
                state.currentFrequency = endFreqs[0];
            } else {
                // No other buttons are held, stop the chord
                Serial.println("Stopping chord (no other buttons held)");
                for (int j = 0; j < 4; j++) {
                    if (state.currentChordNotes[j] != -1) {
                        Serial.print("Stopping voice ");
                        Serial.print(j);
                        Serial.print(": MIDI ");
                        Serial.println(state.currentChordNotes[j]);
                        sendMidiNoteOff(state.currentChordNotes[j], 0, MIDI_CHANNEL);
                        stopNote(j);
                        state.currentChordNotes[j] = -1;
                        state.currentChordFrequencies[j] = 0.0;  // Reset frequency
                        state.waveformOpen[j] = 1;
                    }
                }
                state.currentButton = -1;
                state.currentFrequency = 0.0;
            }
        }
    }

    // Check for pitchBend changes while a chord is playing
    static int prevPitchBend = 0;
    if (state.pitchBend != prevPitchBend && state.currentButton != -1) {
        // Stop the current chord
        for (int i = 0; i < 4; i++) {
            if (state.currentChordNotes[i] != -1) {
                Serial.print("Stopping voice ");
                Serial.print(i);
                Serial.print(": MIDI ");
                Serial.println(state.currentChordNotes[i]);
                sendMidiNoteOff(state.currentChordNotes[i], 0, MIDI_CHANNEL);
                stopNote(i);
                state.currentChordNotes[i] = -1;
                state.waveformOpen[i] = 1;
            }
        }

        // Play the chord with the new pitchBend
        int chordNotes[4];
        int numNotes;
        getChordNotes(state, state.currentButton + 1, chordNotes, numNotes);  // 1-based scale degree

        float startFreqs[4] = { 0 };
        float endFreqs[4] = { 0 };
        Serial.print("Playing chord for button ");
        Serial.print(state.currentButton);
        Serial.print(" with new pitchBend: ");
        for (int i = 0; i < numNotes; i++) {
            int midiNote = chordNotes[i] + state.pitchBend;
            if (midiNote < 0) midiNote = 0;
            if (midiNote > 127) midiNote = 127;

            Serial.print("Voice ");
            Serial.print(i);
            Serial.print(": MIDI ");
            Serial.print(midiNote);
            if (i < numNotes - 1) Serial.print(", ");
            sendMidiNoteOn(midiNote, MIDI_VELOCITY, MIDI_CHANNEL);
            playNote(state, i, midiNote);  // Play the note immediately
            state.currentChordNotes[i] = midiNote;
            endFreqs[i] = midiToPitchFloat[midiNote];  // Store the target frequency
            startFreqs[i] = state.currentChordFrequencies[i];  // Use the actual frequency of the current note
            state.waveformOpen[i] = 0;
        }
        Serial.println();

        // Apply portamento to all notes in the chord
        if (state.portamentoEnabled) {
            Serial.println("Starting portamento for chord with pitchBend");
            for (int i = 0; i < numNotes; i++) {
                Serial.print("Voice ");
                Serial.print(i);
                Serial.print(": ");
                Serial.print(startFreqs[i]);
                Serial.print(" Hz to ");
                Serial.print(endFreqs[i]);
                Serial.println(" Hz");
            }
            for (int i = 0; i <= PORTAMENTO_STEPS; i++) {
                float t = (float)i / PORTAMENTO_STEPS;  // Linear interpolation (0 to 1)
                for (int j = 0; j < numNotes; j++) {
                    float freq = startFreqs[j] + t * (endFreqs[j] - startFreqs[j]);  // Linear glide
                    waveformMod[j].frequency(freq);  // Use the correct voice for each note
                }
                delay(PORTAMENTO_DELAY_MS);  // Adjusted delay for slower glide
            }
        } else {
            // Immediate change (no portamento)
            for (int i = 0; i < numNotes; i++) {
                playNote(state, i, state.currentChordNotes[i]);  // Play the note immediately
            }
        }

        // Update the frequencies after portamento (or immediate change)
        for (int i = 0; i < numNotes; i++) {
            state.currentChordFrequencies[i] = endFreqs[i];  // Store the actual frequency
        }
        state.currentFrequency = endFreqs[0];
    }
    prevPitchBend = state.pitchBend;
}