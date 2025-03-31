// audio.cpp
// Implements audio functions for the SNES synthesizer, including audio setup
// and note playback using the Teensy Audio Library.

#include "audio.h"
#include "debug.h"
#include <Audio.h>

// Audio components (4 voices)
AudioSynthWaveform waveform[4];  // Waveforms for each voice
AudioSynthWaveformModulated waveformMod[4];  // Modulated waveforms for each voice
AudioEffectEnvelope envelope[4];  // Envelopes for each voice
AudioMixer4 mixer;  // Mixer to combine all voices
AudioOutputI2S i2s1;  // I2S output
AudioConnection* patchCords[12];  // 4 voices to mixer (4), mixer to left (1), mixer to right (1), plus waveform to waveformMod (4) and waveformMod to envelope (4)
AudioControlSGTL5000 sgtl5000_1;  // Audio shield

// Keep track of current frequencies for portamento
static float voiceFrequencies[4] = {0, 0, 0, 0};  // Current frequency
static float targetFrequencies[4] = {0, 0, 0, 0};  // Target frequency
static float previousFrequencies[4] = {0, 0, 0, 0};  // Previous frequency (for returning on release)
static bool portamentoActive[4] = {false, false, false, false};
static bool voiceActive[4] = {false, false, false, false};  // Track if voice is currently playing
static const float PORTAMENTO_RATE = 0.008;  // Rate of frequency change (Slowed down again from 0.02)

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
    AudioMemory(40);  // Increase memory allocation to 40 blocks

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
        waveformMod[i].amplitude(0.5);  // Lower amplitude to avoid clipping
        envelope[i].attack(10);
        envelope[i].decay(200);
        envelope[i].sustain(1.0);
        envelope[i].release(50);  // Reduce release time to 50ms for faster stop

        // Set up patch cords for each voice
        DEBUG_DEBUG(CAT_AUDIO, "Setting up patch cords for voice %d", i);
        patchCords[i*3] = new AudioConnection(waveform[i], 0, waveformMod[i], 0);
        patchCords[i*3 + 1] = new AudioConnection(waveformMod[i], 0, envelope[i], 0);
        patchCords[i*3 + 2] = new AudioConnection(envelope[i], 0, mixer, i);  // Route each voice to the mixer
    }

    // Set mixer gains (lower to prevent clipping)
    DEBUG_DEBUG(CAT_AUDIO, "Setting mixer gains");
    for (int i = 0; i < 4; i++) {
        mixer.gain(i, 0.15);  // Total gain = 0.15 * 4 = 0.6
    }

    // Route the mixer output to both left and right channels
    DEBUG_DEBUG(CAT_AUDIO, "Setting up stereo output");
    patchCords[8] = new AudioConnection(mixer, 0, i2s1, 0);  // Mixer to left channel
    patchCords[9] = new AudioConnection(mixer, 0, i2s1, 1);  // Mixer to right channel

    DEBUG_INFO(CAT_AUDIO, "Audio setup complete");
}

void playNote(SynthState& state, int voice, int midiNote) {
    float freq = 440.0 * pow(2.0, (midiNote - 69.0) / 12.0);  // Convert MIDI note to frequency
    
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
    DEBUG_INFO(CAT_AUDIO, "Stopping voice %d", voice);
    envelope[voice].noteOff();
    voiceActive[voice] = false;  // Mark voice as inactive
    
    // If portamento is active, start sliding back to the previous frequency
    if (portamentoActive[voice] && previousFrequencies[voice] > 0) {
        targetFrequencies[voice] = previousFrequencies[voice];
        DEBUG_DEBUG(CAT_AUDIO, "Portamento return on voice %d to %f", voice, previousFrequencies[voice]);
    }
}

// Call this function in your main loop
void updateAudio(SynthState& state) {
    if (state.portamentoEnabled) {
        updatePortamento();
    }
}