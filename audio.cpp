// audio.cpp
// Implements audio functions for the SNES synthesizer, including audio setup
// and note playback using the Teensy Audio Library.

#include "audio.h"
#include <Audio.h>

// Audio components (4 voices)
AudioSynthWaveform waveform[4];  // Waveforms for each voice
AudioSynthWaveformModulated waveformMod[4];  // Modulated waveforms for each voice
AudioEffectEnvelope envelope[4];  // Envelopes for each voice
AudioMixer4 mixer;  // Mixer to combine all voices
AudioOutputI2S i2s1;  // I2S output
AudioConnection* patchCords[12];  // 4 voices to mixer (4), mixer to left (1), mixer to right (1), plus waveform to waveformMod (4) and waveformMod to envelope (4)

void setupAudio() {
    AudioMemory(40);  // Increase memory allocation to 40 blocks

    // Initialize 4 voices
    for (int i = 0; i < 4; i++) {
        waveform[i].begin(WAVEFORM_SINE);
        waveform[i].amplitude(0.5);  // Lower amplitude to avoid clipping
        waveformMod[i].begin(WAVEFORM_SINE);
        waveformMod[i].amplitude(0.5);  // Lower amplitude to avoid clipping
        envelope[i].attack(10);
        envelope[i].decay(200);
        envelope[i].sustain(1.0);
        envelope[i].release(50);  // Reduce release time to 50ms for faster stop

        // Set up patch cords for each voice
        patchCords[i*3] = new AudioConnection(waveform[i], 0, waveformMod[i], 0);
        patchCords[i*3 + 1] = new AudioConnection(waveformMod[i], 0, envelope[i], 0);
        patchCords[i*3 + 2] = new AudioConnection(envelope[i], 0, mixer, i);  // Route each voice to the mixer
    }

    // Set mixer gains (lower to prevent clipping)
    for (int i = 0; i < 4; i++) {
        mixer.gain(i, 0.15);  // Total gain = 0.15 * 4 = 0.6
    }

    // Route the mixer output to both left and right channels
    patchCords[8] = new AudioConnection(mixer, 0, i2s1, 0);  // Mixer to left channel
    patchCords[9] = new AudioConnection(mixer, 0, i2s1, 1);  // Mixer to right channel
}

void playNote(SynthState& state, int voice, int midiNote) {
    float freq = 440.0 * pow(2.0, (midiNote - 69.0) / 12.0);  // Convert MIDI note to frequency
    waveformMod[voice].frequency(freq);
    envelope[voice].noteOn();
    Serial.print("Audio: Playing voice ");
    Serial.print(voice);
    Serial.print(" at frequency ");
    Serial.print(freq);
    Serial.print(", envelope state: ");
    Serial.println(envelope[voice].isActive() ? "Active" : "Inactive");
}

void stopNote(int voice) {
    envelope[voice].noteOff();
    Serial.print("Audio: Stopping voice ");
    Serial.println(voice);
}