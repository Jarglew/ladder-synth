#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>

namespace stack
{

enum class FxType { distortion = 0, chorus, phaser, delay, reverb };
constexpr int numFx = 5;

struct FxSettings
{
    bool distOn = false;
    float distDrive = 0.4f, distTone = 0.6f, distMix = 1.0f;

    bool chorusOn = false;
    float chorusRate = 0.6f, chorusDepth = 0.4f, chorusMix = 0.5f;

    bool phaserOn = false;
    float phaserRate = 0.3f, phaserDepth = 0.6f, phaserFeedback = 0.4f, phaserMix = 0.5f;

    bool delayOn = false;
    double delayBeats = 0.75;   // already converted from the note-value choice
    float delayFeedback = 0.35f, delayMix = 0.25f;
    bool delayPingPong = true;

    bool reverbOn = false;
    float reverbSize = 0.6f, reverbDamping = 0.5f, reverbWidth = 1.0f, reverbMix = 0.25f;

    std::array<int, numFx> order { 0, 1, 2, 3, 4 };   // FxType values, first to last
};

// Stereo effects in a user-chosen order, then a soft safety limiter.
class FxChain
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const FxSettings& settings, double bpm);

private:
    double sampleRate = 44100.0;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::Reverb reverb;

    // Distortion tone filter state
    float toneStateL = 0.0f, toneStateR = 0.0f;

    // Delay
    std::vector<float> delayL, delayR;
    int delayWrite = 0;
    float delayDampL = 0.0f, delayDampR = 0.0f;
    float smoothedDelaySamples = -1.0f;

    void distortion (juce::AudioBuffer<float>&, const FxSettings&);
    void runChorus (juce::AudioBuffer<float>&, const FxSettings&);
    void runPhaser (juce::AudioBuffer<float>&, const FxSettings&);
    void delay (juce::AudioBuffer<float>&, const FxSettings&, double bpm);
    void runReverb (juce::AudioBuffer<float>&, const FxSettings&);
    static void limiter (juce::AudioBuffer<float>&);
};

} // namespace stack
