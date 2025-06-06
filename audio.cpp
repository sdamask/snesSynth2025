// audio.cpp
// Implements audio functions for the SNES synthesizer, including audio setup
// and note playback using the Teensy Audio Library.

#include "audio.h"
#include "debug.h"
#include <Audio.h>
#include "synth_state.h"
#include "button_defs.h"
#include "playstyles.h" // Include to get thunderstruckMidiNotes declaration
#include "utils.h" // For midiToPitchFloat
#include "midi.h" // Include for sendMidiNoteOn/Off

// Audio components (4 voices)
AudioSynthWaveform waveform[4];  // Waveforms for each voice
AudioSynthWaveformModulated waveformMod[4];  // Modulated waveforms for each voice
AudioEffectEnvelope envelope[4];  // Envelopes for each voice
AudioSynthWaveform lfo[4];          // LFOs for vibrato
AudioMixer4 mixer;  // Mixer to combine all voices
AudioOutputI2S i2s1;  // I2S output
AudioConnection* patchCords[16];  // Increased size from 12 to 16 
AudioControlSGTL5000 sgtl5000_1;  // Audio shield

// Keep track of current frequencies for portamento
static float voiceFrequencies[4] = {0, 0, 0, 0};  // Current frequency
static float targetFrequencies[4] = {0, 0, 0, 0};  // Target frequency
static float previousFrequencies[4] = {0, 0, 0, 0};  // Previous frequency (for returning on release)
static bool portamentoActive[4] = {false, false, false, false};
static bool voiceActive[4] = {false, false, false, false};  // Track if voice is currently playing
static const float PORTAMENTO_RATE = 0.008;  // Rate of frequency change (Slowed down again from 0.02)

// Helper array to map state index to Teensy waveform constants
const int waveformTypes[] = {
    WAVEFORM_SINE, 
    WAVEFORM_SAWTOOTH, 
    WAVEFORM_SQUARE, 
    WAVEFORM_TRIANGLE
};

// Vibrato settings
const float VIBRATO_RATES[] = {0.0, 5.0, 10.0}; // Hz (0=Off)
// Depth now represents LFO Amplitude (0.0 to 1.0)
// Adjusted values for Low/Medium/High intensity control via LFO amplitude
const float VIBRATO_DEPTHS[] = {0.0, 0.1, 0.3, 0.7}; // LFO Amplitude (0=Off)

void updatePortamento() {
    for (int i = 0; i < 4; i++) {
        if (portamentoActive[i] && voiceFrequencies[i] != targetFrequencies[i]) {
            // Calculate the difference between current and target frequencies
            float diff = targetFrequencies[i] - voiceFrequencies[i];
            
            // Move current frequency towards target by a percentage of the difference
            voiceFrequencies[i] += diff * PORTAMENTO_RATE;
            
            // If we're very close to the target, snap to it
            if (abs(diff) < 0.1) {
                voiceFrequencies[i] = targetFrequencies[i];
                portamentoActive[i] = false;
            }
            
            // Update the actual frequency
            waveformMod[i].frequency(voiceFrequencies[i]);
            // DEBUG_INFO(CAT_AUDIO, "Portamento update voice %d: curr=%f target=%f diff=%f rate=%f", i, voiceFrequencies[i], targetFrequencies[i], diff, PORTAMENTO_RATE);
        }
    }
}

void setupAudio() {
    DEBUG_INFO(CAT_AUDIO, "Allocating audio memory (40 blocks)");
    AudioMemory(40); 

    // Enable the audio shield
    DEBUG_INFO(CAT_AUDIO, "Enabling audio shield");
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.5);  // Set master volume to 50%
    sgtl5000_1.lineOutLevel(13);  // Set line out level (0-31)

    // Initialize 4 voices
    for (int i = 0; i < 4; i++) {
        DEBUG_DEBUG(CAT_AUDIO, "Initializing voice %d", i);
        waveform[i].begin(WAVEFORM_SINE);
        waveform[i].amplitude(0.5);  // Lower amplitude to avoid clipping
        waveformMod[i].begin(WAVEFORM_SINE);
        waveformMod[i].amplitude(0.5); 
        waveformMod[i].frequencyModulation(0.1); 
        envelope[i].attack(10);
        envelope[i].decay(200);
        envelope[i].sustain(1.0);
        envelope[i].release(1);  // Set release time to minimum (1ms)

        // Setup LFO for vibrato
        lfo[i].begin(0.0, 0.0, WAVEFORM_SINE); 

        // Set up patch cords for each voice
        // patchCords[i*4] = REMOVED
        patchCords[i*4 + 1] = new AudioConnection(lfo[i], 0, waveformMod[i], 0); // LFO -> Freq Mod Input (Input 0)
        patchCords[i*4 + 2] = new AudioConnection(waveformMod[i], 0, envelope[i], 0); // Modulated -> Envelope
        patchCords[i*4 + 3] = new AudioConnection(envelope[i], 0, mixer, i);  // Envelope -> Mixer
    }

    // Set mixer gains
    DEBUG_DEBUG(CAT_AUDIO, "Setting mixer gains");
    mixer.gain(0, 0.24); // Increased gain for voice 0 (Monophonic)
    DEBUG_DEBUG(CAT_AUDIO, "  - Mixer gain 0 set to 0.24");
    for (int i = 1; i < 4; i++) { // Set gain for voices 1, 2, 3 (Chord/Poly)
        mixer.gain(i, 0.12);
        DEBUG_DEBUG(CAT_AUDIO, "  - Mixer gain %d set to 0.12", i);
    }

    // Route the mixer output to both left and right channels
    patchCords[12] = new AudioConnection(mixer, 0, i2s1, 0); // Mixer to left
    patchCords[13] = new AudioConnection(mixer, 0, i2s1, 1); // Mixer to right
    // Indices 14, 15 unused

    DEBUG_INFO(CAT_AUDIO, "Audio setup complete");
}

// Function to apply vibrato settings based on state
void applyVibrato(SynthState& state, int voice) {
     // REMOVED: Waveform begin() call moved back to setupAudio and playNote

     // Apply vibrato rate/depth (now LFO amplitude)
     if (state.vibratoRate > 0 && state.vibratoDepth > 0) {
         float rate = VIBRATO_RATES[state.vibratoRate];
         float depth_amplitude = VIBRATO_DEPTHS[state.vibratoDepth]; // Use amplitude depth value
         lfo[voice].frequency(rate);
         lfo[voice].amplitude(depth_amplitude); // CONTROL intensity via LFO amplitude
         // REMOVED: Modulation depth is fixed now
         Serial.printf("  applyVibrato: Voice=%d Rate=%.2f LFO_Amp=%.4f (Applied)\n", voice, rate, depth_amplitude);
     } else {
         lfo[voice].amplitude(0.0); // Turn LFO off by setting amplitude to 0
         // REMOVED: Modulation depth is fixed now
         // waveformMod[voice].frequencyModulation(0.0);
         Serial.printf("  applyVibrato: Voice=%d LFO OFF (0.0 Amplitude Applied)\n", voice);
     }
}

void playNote(SynthState& state, int voice, int midiNote) {
    float freq = 440.0 * pow(2.0, (midiNote - 69.0) / 12.0);  
    DEBUG_INFO(CAT_AUDIO, ">>> playNote called: voice=%d, midiNote=%d, freq=%.2f", voice, midiNote, freq); // <<< ADDED DEBUG
    
    // Set Waveform Type (can potentially reset modulation depth? Keep testing)
    int selectedWaveformType = waveformTypes[state.currentWaveform];
    waveformMod[voice].begin(selectedWaveformType);

    // Apply Vibrato Settings (Rate and LFO Amplitude)
    applyVibrato(state, voice); 

    // --- Portamento Logic --- 
    if (state.portamentoEnabled) {
        if (!voiceActive[voice]) {
            // First note after initialization or stopping - set frequency directly
            voiceFrequencies[voice] = freq;
            targetFrequencies[voice] = freq;
            previousFrequencies[voice] = freq;
            portamentoActive[voice] = false;
            waveformMod[voice].frequency(freq);
            DEBUG_DEBUG(CAT_AUDIO, "Initial frequency set on voice %d: %f", voice, freq);
        } else {
            // Store current frequency as previous before updating target
            previousFrequencies[voice] = voiceFrequencies[voice];
            
            // Set up portamento - keep current frequency and set new target
            targetFrequencies[voice] = freq;
            portamentoActive[voice] = true;
            DEBUG_DEBUG(CAT_AUDIO, "Portamento active on voice %d: %f -> %f", voice, voiceFrequencies[voice], freq);
        }
    } else {
        // No portamento, set frequency directly
        previousFrequencies[voice] = freq;  // Store for future portamento
        voiceFrequencies[voice] = freq;
        targetFrequencies[voice] = freq;
        portamentoActive[voice] = false;
        waveformMod[voice].frequency(freq);
        DEBUG_DEBUG(CAT_AUDIO, "Direct frequency set on voice %d: %f", voice, freq);
    }
    
    envelope[voice].noteOn();
    voiceActive[voice] = true;  // Mark voice as active
    
    if (!envelope[voice].isActive()) {
        DEBUG_WARNING(CAT_AUDIO, "Voice %d envelope not active after noteOn", voice);
    }
}

void stopNote(int voice) {
    DEBUG_INFO(CAT_AUDIO, ">>> stopNote called: voice=%d", voice); // <<< ADDED DEBUG
    DEBUG_VERBOSE(CAT_AUDIO, "Stopping voice %d", voice);
    envelope[voice].noteOff();
    voiceActive[voice] = false;  // Mark voice as inactive
    lfo[voice].amplitude(0.0); // Stop LFO output
    
    // If portamento is active, start sliding back to the previous frequency
    if (portamentoActive[voice] && previousFrequencies[voice] > 0) {
        targetFrequencies[voice] = previousFrequencies[voice];
        DEBUG_VERBOSE(CAT_AUDIO, "Portamento return on voice %d to %f", voice, previousFrequencies[voice]);
    }
}

// Call this function in your main loop
void updateAudio(SynthState& state) {
    if (state.portamentoEnabled) {
        updatePortamento();
    }
}

// Helper function to get the current base MIDI note from pressed buttons
// V12.1 - Uses lastPressedBuffer for priority (most recent held)
int getBaseMidiNote(SynthState& state) {
    int buttonToPlay = -1;
    int readIndex = state.lastPressedIndex; // Start reading from the element AFTER the last one written

    // Iterate backwards through the buffer to find the most recent press
    for (int i = 0; i < LAST_PRESS_BUFFER_SIZE; ++i) {
        // Decrement index, wrap around buffer
        readIndex = (readIndex + LAST_PRESS_BUFFER_SIZE - 1) % LAST_PRESS_BUFFER_SIZE;
        int bufferedButton = state.lastPressedBuffer[readIndex];

        // Check if buffer slot is valid (>=0) AND the button is STILL held
        if (bufferedButton >= 0 && bufferedButton < MAX_NOTE_BUTTONS && state.held[bufferedButton]) {
            buttonToPlay = bufferedButton; // Found the most recent, still-held button
            DEBUG_VERBOSE(CAT_AUDIO, "getBaseMidiNote: Found most recent held button %d from buffer.", buttonToPlay);
            break; // Stop searching, priority found
        }
    }

    // If no held button was found by checking the buffer, return -1
    if (buttonToPlay == -1) {
        // Optional: Double check state.held[] directly as a fallback? Could be noisy.
        // For now, trust the buffer check combined with current held state.
        DEBUG_VERBOSE(CAT_AUDIO, "getBaseMidiNote: No valid held button found in recent press buffer. Returning -1.");
        return -1;
    }

    // --- Found a prioritized button, proceed to get its note --- 
    DEBUG_VERBOSE(CAT_AUDIO, "getBaseMidiNote: Prioritizing button index=%d.", buttonToPlay);
    if (state.customProfileIndex == PROFILE_THUNDERSTRUCK) {
        // Handle Thunderstruck profile (Check bounds just in case)
        if (buttonToPlay >= 0 && buttonToPlay < MAX_NOTE_BUTTONS) {
            int note = thunderstruckMidiNotes[buttonToPlay];
            DEBUG_VERBOSE(CAT_AUDIO, "  -> Thunderstruck profile: Returning note %d", note);
            return note;
        } else {
            DEBUG_WARNING(CAT_AUDIO, "getBaseMidiNote: Invalid button index %d for Thunderstruck.", buttonToPlay);
            return -1;
        }
    } else {
        // Handle Scale profile
        if (buttonToPlay >= 0 && buttonToPlay < MAX_NOTE_BUTTONS) {
            int musicalPosition = buttonToMusicalPosition[buttonToPlay];
            DEBUG_VERBOSE(CAT_AUDIO, "  -> Scale profile: Button %d maps to musicalPos %d", buttonToPlay, musicalPosition);
            if (musicalPosition >= 0 && musicalPosition < 12) {
                 int note = state.scaleHolder[musicalPosition];
                 DEBUG_VERBOSE(CAT_AUDIO, "     -> Scale lookup: Returning note %d", note);
                 return note;
            } else {
                DEBUG_WARNING(CAT_AUDIO, "getBaseMidiNote: Invalid musicalPosition %d for button %d", musicalPosition, buttonToPlay);
                return -1;
            }
        } else {
             DEBUG_WARNING(CAT_AUDIO, "getBaseMidiNote: Invalid button index %d for Scale profile.", buttonToPlay);
             return -1;
        }
    }
    // Should not be reached, but added for safety
    // return -1; 
}