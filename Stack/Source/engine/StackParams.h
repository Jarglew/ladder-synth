#pragma once

// Plain settings snapshot for the Layer Stack engine. No JUCE types, so the
// engine builds and tests on its own.

#include <array>
#include <cstddef>

namespace stack
{

constexpr int numLayers = 4;
constexpr int maxSteps = 16;

enum class LayerWave
{
    saw = 0,
    square,
    triangle,
    sine,
    supersaw,
    noise,
    sample
};

enum class FilterType
{
    off = 0,
    lp24,   // transistor ladder
    lp12,
    hp12,
    bp12
};

enum class LfoShape
{
    sine = 0,
    triangle,
    saw,
    square,
    sampleHold
};

// Musical step lengths shared by the arpeggiator, trance gate and delay.
enum class NoteRate
{
    quarter = 0,
    eighth,
    eighthTriplet,
    sixteenth,
    sixteenthTriplet,
    thirtySecond
};

inline double rateInBeats (NoteRate r)
{
    switch (r)
    {
        case NoteRate::quarter:          return 1.0;
        case NoteRate::eighth:           return 0.5;
        case NoteRate::eighthTriplet:    return 1.0 / 3.0;
        case NoteRate::sixteenth:        return 0.25;
        case NoteRate::sixteenthTriplet: return 1.0 / 6.0;
        case NoteRate::thirtySecond:     return 0.125;
    }
    return 0.25;
}

enum class ArpMode
{
    up = 0,
    down,
    upDown,
    downUp,
    random,
    asPlayed,
    chord
};

enum class VoiceMode
{
    poly = 0,
    mono
};

struct EnvelopeTimes
{
    float attack = 0.005f, decay = 0.3f, sustain = 0.8f, release = 0.3f;
};

struct LayerSettings
{
    bool enabled = false;
    LayerWave wave = LayerWave::saw;
    int octave = 0;            // -3..+3
    int semitone = 0;          // -12..+12
    float fineCents = 0.0f;    // -100..+100
    float detune = 0.3f;       // supersaw spread, 0..1
    float width = 0.6f;        // supersaw stereo spread, 0..1
    float level = 0.8f;
    float pan = 0.0f;          // -1..1
    FilterType filterType = FilterType::lp24;
    float cutoffHz = 8000.0f;
    float resonance = 0.1f;
    float filterEnvAmount = 0.0f;  // -1..1, up to +-6 octaves
    float keyTrack = 0.5f;         // 0..1
    EnvelopeTimes filterEnv { 0.005f, 0.4f, 0.3f, 0.3f };
    EnvelopeTimes ampEnv { 0.005f, 0.3f, 0.8f, 0.3f };
    float velocity = 0.5f;     // velocity sensitivity, 0..1
    int sampleRoot = 60;       // MIDI note the sample was recorded at
    bool sampleLoop = false;
};

struct ArpSettings
{
    bool enabled = false;
    ArpMode mode = ArpMode::up;
    NoteRate rate = NoteRate::sixteenth;
    int octaves = 1;           // 1..4
    float gate = 0.6f;         // fraction of a step, 0.05..1
    float swing = 0.0f;        // 0..1 (0 = straight, 1 = heavy shuffle)
    bool latch = false;
    int steps = 16;            // 1..16
    std::array<bool, maxSteps> stepOn {};
    std::array<float, maxSteps> stepVelocity {};

    ArpSettings()
    {
        stepOn.fill (true);
        stepVelocity.fill (1.0f);
    }
};

struct GateSettings
{
    bool enabled = false;
    NoteRate rate = NoteRate::sixteenth;
    int steps = 16;
    float depth = 1.0f;        // how far closed steps duck, 0..1
    float attackMs = 1.0f;
    float releaseMs = 12.0f;
    std::array<bool, maxSteps> stepOn {};

    GateSettings()
    {
        for (int i = 0; i < maxSteps; ++i)
            stepOn[(std::size_t) i] = (i % 2 == 0);
    }
};

struct StackParams
{
    std::array<LayerSettings, numLayers> layers;
    ArpSettings arp;
    GateSettings gate;

    VoiceMode voiceMode = VoiceMode::poly;
    float glideSeconds = 0.0f;
    float bendRangeSemitones = 2.0f;

    LfoShape lfo1Shape = LfoShape::sine;
    float lfo1RateHz = 0.5f;
    float lfo1ToCutoff = 0.0f;     // -1..1, up to +-3 octaves
    LfoShape lfo2Shape = LfoShape::sine;
    float lfo2RateHz = 5.5f;
    float lfo2ToPitch = 0.0f;      // 0..1, up to 1 semitone
    float modWheelToVibrato = 0.5f;

    float volumeGain = 0.5f;

    StackParams()
    {
        layers[0].enabled = true;
    }
};

// Host transport, passed in every block.
struct Transport
{
    double bpm = 120.0;
    bool playing = false;
    double ppqAtBlockStart = 0.0;  // only meaningful when playing
};

} // namespace stack
