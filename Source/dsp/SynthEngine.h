#pragma once

#include "SynthParams.h"
#include "Voice.h"
#include <array>
#include <cstdint>
#include <vector>

namespace ladder
{

// Owns the voices and turns note events into voice actions.
// Mono mode: last-note priority with a held-note stack, optional legato.
// Poly mode: up to maxVoices notes, stealing released then oldest voices.
class SynthEngine
{
public:
    static constexpr int maxVoices = 8;

    void prepare (double sampleRate);
    void setParams (const SynthParams& newParams);
    const SynthParams& getParams() const { return params; }

    void noteOn (int note, float velocity);   // velocity 0..1
    void noteOff (int note);
    void setSustainPedal (bool down);
    void setPitchBend (float normalised);     // -1..1
    void setModWheel (float value);           // 0..1
    void allNotesOff();                       // release everything
    void reset();                             // silence immediately

    // Renders mono output, overwriting numSamples values in out.
    void render (float* out, int numSamples);

    int activeVoiceCount() const;

private:
    SynthParams params;
    PerformanceState perf;
    std::array<Voice, maxVoices> voices;
    float bendNormalised = 0.0f;

    // Mono note handling
    std::vector<int> monoStack;                 // notes held or sustained, last = sounding
    std::array<bool, 128> physicallyHeld {};
    std::array<float, 128> heldVelocity {};
    bool sustainDown = false;
    std::uint64_t ageCounter = 0;

    void monoNoteOn (int note, float velocity);
    void monoNoteOff (int note);
    void monoUpdateAfterRemoval (int removedNote);
    void polyNoteOn (int note, float velocity);
    void polyNoteOff (int note);
    Voice& findVoiceToUse();
    bool glideOn() const { return params.glideSeconds > 0.0005f; }
};

} // namespace ladder
