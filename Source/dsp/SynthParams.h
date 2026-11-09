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

// ---------------------------------------------------------------- performance and FX
// These are handled by Instrument, around the voice engine.

enum class ArpMode
{
    up = 0,
    down,
    upDown,
    random,
    asPlayed,
    chord
};

// Musical note lengths for tempo-synced things, in beats (quarter notes).
enum class NoteLength
{
    whole = 0,
    half,
    quarterDotted,
    quarter,
    quarterTriplet,
    eighthDotted,
    eighth,
    eighthTriplet,
    sixteenthDotted,
    sixteenth,
    sixteenthTriplet,
    thirtySecond
};

inline double noteLengthBeats (NoteLength n)
{
    switch (n)
    {
        case NoteLength::whole:            return 4.0;
        case NoteLength::half:             return 2.0;
        case NoteLength::quarterDotted:    return 1.5;
        case NoteLength::quarter:          return 1.0;
        case NoteLength::quarterTriplet:   return 2.0 / 3.0;
        case NoteLength::eighthDotted:     return 0.75;
        case NoteLength::eighth:           return 0.5;
        case NoteLength::eighthTriplet:    return 1.0 / 3.0;
        case NoteLength::sixteenthDotted:  return 0.375;
        case NoteLength::sixteenth:        return 0.25;
        case NoteLength::sixteenthTriplet: return 1.0 / 6.0;
        case NoteLength::thirtySecond:     return 0.125;
    }
    return 0.25;
}

struct ArpSettings
{
    bool enabled = false;
    ArpMode mode = ArpMode::up;
    NoteLength rate = NoteLength::sixteenth;
    int octaves = 1;           // 1..4
    float gate = 0.5f;         // note length as a fraction of a step, 0.05..1
    float swing = 0.0f;        // 0..1, delays every second step by up to half a step
    bool hold = false;         // latch: notes keep playing after the keys are let go
};

struct GateSettings
{
    static constexpr int numSteps = 16;

    bool enabled = false;
    NoteLength rate = NoteLength::sixteenth;
    float depth = 1.0f;        // 0..1, how far closed steps duck the level
    float length = 0.75f;      // 0.1..1, open part of each open step
    float smooth = 0.2f;       // 0..1, edge softness
    bool steps[numSteps] { true, false, true, true, false, true, true, false,
                           true, false, true, true, false, true, true, false };
};

struct ChorusSettings
{
    bool enabled = false;
    float rateHz = 0.6f;
    float depth = 0.5f;        // 0..1
    float mix = 0.5f;          // 0..1
};

struct DelaySettings
{
    bool enabled = false;
    NoteLength time = NoteLength::eighthDotted;
    float feedback = 0.35f;    // 0..0.9
    float damping = 0.3f;      // 0..1, darkens each repeat
    float mix = 0.25f;         // 0..1
    bool pingPong = true;
};

struct ReverbSettings
{
    bool enabled = false;
    float size = 0.6f;         // 0..1
    float damping = 0.4f;      // 0..1
    float width = 1.0f;        // 0..1
    float mix = 0.25f;         // 0..1
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

    ArpSettings arp;
    GateSettings gate;
    ChorusSettings chorus;
    DelaySettings delay;
    ReverbSettings reverb;
};

} // namespace ladder
