#pragma once

#include "Arpeggiator.h"
#include "LayerVoice.h"
#include "SampleData.h"
#include "StackParams.h"
#include "TranceGate.h"
#include <array>
#include <cstdint>
#include <vector>

namespace stack
{

// A MIDI-like event coming into the engine.
struct InputEvent
{
    enum class Type { noteOn, noteOff, pitchBend, modWheel, sustain, allNotesOff, allSoundOff };
    Type type = Type::noteOn;
    int offset = 0;
    int note = 0;
    float value = 0.0f;   // velocity 0..1, bend -1..1, mod wheel 0..1, sustain 0/1
};

// One playing note: four layers sharing a pitch.
class StackVoice
{
public:
    void prepare (double sampleRate, std::uint32_t seed);
    void reset();
    void start (const StackParams& p, const std::array<const SampleData*, numLayers>& samples, int note,
                float velocity, bool glide, std::uint64_t age);
    void glideTo (int note, std::uint64_t age);
    void release();
    void kill();
    void killLayer (int layer) { layers[(std::size_t) layer].kill(); }

    bool isActive() const;
    bool isGateOn() const { return gate; }
    int getNote() const { return note; }
    std::uint64_t getAge() const { return age; }

    void render (const StackParams& p, float bendSemis, float vibratoDepth, const float* lfo1, const float* lfo2,
                 float* outL, float* outR, int numSamples);

private:
    std::array<LayerVoice, numLayers> layers;
    double sampleRate = 44100.0;
    int note = -1;
    bool gate = false;
    bool hasPlayed = false;
    std::uint64_t age = 0;
    double currentPitch = 60.0, targetPitch = 60.0;
};

class StackEngine
{
public:
    static constexpr int maxVoices = 16;

    void prepare (double sampleRate, int maxBlockSize);
    void setParams (const StackParams& p);
    const StackParams& getParams() const { return params; }

    // The sample a sample-type layer plays. The caller keeps it alive. Changing it
    // silences that layer on every voice, so nothing keeps reading the old sample.
    void setSample (int layer, const SampleData* sample);

    // Renders numSamples of stereo output (overwriting outL/outR). Events must be sorted by offset.
    void process (float* outL, float* outR, int numSamples, const InputEvent* events, int numEvents,
                  const Transport& transport);

    void reset();
    int activeVoiceCount() const;

private:
    StackParams params;
    double sampleRate = 44100.0;
    int maxBlock = 512;

    std::array<StackVoice, maxVoices> voices;
    std::array<const SampleData*, numLayers> samples {};
    Arpeggiator arp;
    NoteEventList arpEvents;
    TranceGate gate;
    Lfo lfo1, lfo2;
    std::vector<float> lfo1Buffer, lfo2Buffer, gateBuffer;

    // Input state
    std::array<bool, 128> physicallyHeld {};
    std::array<bool, 128> sustainedNote {};
    bool sustainDown = false;
    float bendNormalised = 0.0f;
    float modWheel = 0.0f;
    bool arpWasEnabled = false;

    // Mono note stack (last-note priority)
    std::vector<int> monoStack;
    std::array<float, 128> noteVelocity {};

    // Clock
    double internalPpq = 0.0;
    double blockPpq = 0.0;
    double beatsPerSample = 0.0;
    bool hostPlaying = false;
    std::uint64_t ageCounter = 0;

    double ppqAt (int offset) const { return blockPpq + offset * beatsPerSample; }
    void handleInput (const InputEvent& e);
    void segment (float* outL, float* outR, int start, int end);
    void renderVoices (float* outL, float* outR, int start, int end);

    void noteOnInput (int note, float velocity, int offset);
    void noteOffInput (int note);
    void routeNoteOff (int note);
    void voiceNoteOn (int note, float velocity);
    void voiceNoteOff (int note);
    void releaseAllVoices();
    StackVoice& findVoiceToUse();
    bool glideOn() const { return params.glideSeconds > 0.0005f; }
};

} // namespace stack
