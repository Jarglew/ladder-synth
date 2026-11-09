#pragma once

// Plain parameter snapshot handed to the engine once per audio block.
// No JUCE types here, so the engine can be built and tested on its own.

namespace ladder
{

enum class Waveform
{
    triangle = 0,
    triSaw,       // asymmetric "shark tooth" between triangle and saw
    reverseSaw,
    saw,
    square,
    widePulse,
    narrowPulse
};

// Footage ranges, as on the classic oscillator bank. "lo" is a sub-audio range.
enum class Range
{
    lo = 0,
    ft32,
    ft16,
    ft8,
    ft4,
    ft2
};

enum class KeyTrack
{
    off = 0,
    oneThird,
    twoThirds,
    full
};

enum class VoiceMode
{
    mono = 0,
    poly
};

enum class NoiseColour
{
    white = 0,
    pink
};

struct OscillatorSettings
{
    Range range = Range::ft8;
    Waveform wave = Waveform::saw;
    float tuneSemitones = 0.0f; // oscillators 2 and 3 only, -7..+7
};

struct EnvelopeSettings
{
    float attack = 0.005f;  // seconds
    float decay = 0.3f;     // seconds
    float sustain = 0.7f;   // 0..1
    float release = 0.3f;   // seconds
};

struct SynthParams
{
    OscillatorSettings osc[3];
    bool osc3KeyTrack = true;

    float mixOsc[3] = { 1.0f, 0.0f, 0.0f };
    float mixNoise = 0.0f;
    NoiseColour noiseColour = NoiseColour::white;

    float cutoffHz = 2000.0f;
    float emphasis = 0.3f;     // 0..1, self-oscillates near the top
    float contour = 0.5f;      // filter envelope amount, 0..1
    float drive = 0.2f;        // 0..1
    KeyTrack keyTrack = KeyTrack::oneThird;

    EnvelopeSettings filterEnv { 0.005f, 0.4f, 0.3f, 0.3f };
    EnvelopeSettings ampEnv { 0.003f, 0.3f, 0.8f, 0.25f };

    float glideSeconds = 0.0f;
    VoiceMode voiceMode = VoiceMode::mono;
    bool legato = true;

    float modMix = 0.0f;       // 0 = oscillator 3, 1 = noise
    bool oscMod = false;       // modulation to oscillators 1 and 2
    bool filterMod = false;    // modulation to filter cutoff
    float modAmount = 0.0f;    // added to the mod wheel

    float velocityToAmp = 0.5f;
    float velocityToFilter = 0.0f;
    float bendRangeSemitones = 2.0f;
    float masterTuneCents = 0.0f;
    float volumeGain = 0.5f;   // linear
};

} // namespace ladder
