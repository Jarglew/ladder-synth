#include "SynthEngine.h"
#include <algorithm>

namespace ladder
{

void SynthEngine::prepare (double sampleRate)
{
    std::uint32_t seed = 0x1234567u;
    for (auto& v : voices)
    {
        v.prepare (sampleRate);
        v.seedNoise (seed);
        seed = seed * 1664525u + 1013904223u;
    }
    reset();
}

void SynthEngine::setParams (const SynthParams& newParams)
{
    const bool modeChanged = newParams.voiceMode != params.voiceMode;
    params = newParams;
    if (modeChanged)
        allNotesOff();
}

void SynthEngine::noteOn (int note, float velocity)
{
    if (note < 0 || note > 127)
        return;

    if (params.voiceMode == VoiceMode::mono)
        monoNoteOn (note, velocity);
    else
        polyNoteOn (note, velocity);
}

void SynthEngine::noteOff (int note)
{
    if (note < 0 || note > 127)
        return;

    if (params.voiceMode == VoiceMode::mono)
        monoNoteOff (note);
    else
        polyNoteOff (note);
}

void SynthEngine::setSustainPedal (bool down)
{
    if (down == sustainDown)
        return;

    sustainDown = down;
    if (down)
        return;

    // Pedal lifted: let go of every note that is no longer physically held.
    if (params.voiceMode == VoiceMode::mono)
    {
        const int sounding = monoStack.empty() ? -1 : monoStack.back();
        monoStack.erase (std::remove_if (monoStack.begin(), monoStack.end(),
                                         [this] (int n) { return ! physicallyHeld[(size_t) n]; }),
                         monoStack.end());
        monoUpdateAfterRemoval (sounding);
    }
    else
    {
        for (auto& v : voices)
            if (v.isSustained())
                v.release();
    }
}

void SynthEngine::setPitchBend (float normalised)
{
    bendNormalised = std::clamp (normalised, -1.0f, 1.0f);
}

void SynthEngine::setModWheel (float value)
{
    perf.modWheel = std::clamp (value, 0.0f, 1.0f);
}

void SynthEngine::allNotesOff()
{
    for (auto& v : voices)
        if (v.isActive())
            v.release();
    monoStack.clear();
    physicallyHeld.fill (false);
}

void SynthEngine::reset()
{
    for (auto& v : voices)
        v.reset();
    monoStack.clear();
    physicallyHeld.fill (false);
    sustainDown = false;
}

void SynthEngine::render (float* out, int numSamples)
{
    std::fill (out, out + numSamples, 0.0f);
    perf.bendSemitones = bendNormalised * params.bendRangeSemitones;

    for (auto& v : voices)
        v.render (params, perf, out, numSamples);

    for (int i = 0; i < numSamples; ++i)
        out[i] *= params.volumeGain;
}

int SynthEngine::activeVoiceCount() const
{
    return (int) std::count_if (voices.begin(), voices.end(), [] (const Voice& v) { return v.isActive(); });
}

// ---------------------------------------------------------------- mono

void SynthEngine::monoNoteOn (int note, float velocity)
{
    physicallyHeld[(size_t) note] = true;
    heldVelocity[(size_t) note] = velocity;

    const bool wasHolding = ! monoStack.empty();
    monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
    monoStack.push_back (note);

    auto& v = voices[0];
    if (wasHolding && params.legato && v.isActive() && v.isGateOn())
        v.glideTo (note, ++ageCounter);
    else
        v.start (note, velocity, glideOn(), ++ageCounter);
}

void SynthEngine::monoNoteOff (int note)
{
    physicallyHeld[(size_t) note] = false;
    if (sustainDown)
        return;

    monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
    monoUpdateAfterRemoval (note);
}

void SynthEngine::monoUpdateAfterRemoval (int removedNote)
{
    auto& v = voices[0];

    if (monoStack.empty())
    {
        if (v.isGateOn() && v.getNote() == removedNote)
            v.release();
        return;
    }

    // Last-note priority: fall back to the most recent note still held.
    const int top = monoStack.back();
    if (top == v.getNote() && v.isGateOn())
        return;

    if (params.legato && v.isActive())
        v.glideTo (top, ++ageCounter);
    else
        v.start (top, heldVelocity[(size_t) top], glideOn(), ++ageCounter);
}

// ---------------------------------------------------------------- poly

void SynthEngine::polyNoteOn (int note, float velocity)
{
    physicallyHeld[(size_t) note] = true;

    for (auto& v : voices)
    {
        if (v.isActive() && v.getNote() == note)
        {
            v.start (note, velocity, glideOn(), ++ageCounter);
            return;
        }
    }

    findVoiceToUse().start (note, velocity, glideOn(), ++ageCounter);
}

void SynthEngine::polyNoteOff (int note)
{
    physicallyHeld[(size_t) note] = false;

    for (auto& v : voices)
    {
        if (v.isGateOn() && v.getNote() == note)
        {
            if (sustainDown)
                v.setSustained (true);
            else
                v.release();
        }
    }
}

Voice& SynthEngine::findVoiceToUse()
{
    for (auto& v : voices)
        if (! v.isActive())
            return v;

    // Steal the oldest released voice, otherwise the oldest voice.
    Voice* best = nullptr;
    for (auto& v : voices)
        if (! v.isGateOn() && (best == nullptr || v.getAge() < best->getAge()))
            best = &v;

    if (best == nullptr)
        for (auto& v : voices)
            if (best == nullptr || v.getAge() < best->getAge())
                best = &v;

    return *best;
}

} // namespace ladder
