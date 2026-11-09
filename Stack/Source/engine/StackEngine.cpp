#include "StackEngine.h"
#include <algorithm>
#include <cmath>

namespace stack
{

// ================================================================ voice

void StackVoice::prepare (double sr, std::uint32_t seed)
{
    sampleRate = sr;
    for (auto& l : layers)
    {
        l.prepare (sr);
        l.seed (seed);
        seed = seed * 1664525u + 1013904223u;
    }
    reset();
}

void StackVoice::reset()
{
    for (auto& l : layers)
        l.reset();
    note = -1;
    gate = false;
    hasPlayed = false;
}

void StackVoice::start (const StackParams& p, const std::array<const SampleData*, numLayers>& samples, int newNote,
                        float velocity, bool glide, std::uint64_t newAge)
{
    if (! hasPlayed || ! glide)
        currentPitch = newNote;
    targetPitch = newNote;
    note = newNote;
    gate = true;
    hasPlayed = true;
    age = newAge;

    for (int i = 0; i < numLayers; ++i)
        if (p.layers[(std::size_t) i].enabled)
            layers[(std::size_t) i].start (p.layers[(std::size_t) i], samples[(std::size_t) i], velocity);
}

void StackVoice::glideTo (int newNote, std::uint64_t newAge)
{
    note = newNote;
    targetPitch = newNote;
    gate = true;
    age = newAge;
}

void StackVoice::release()
{
    gate = false;
    for (auto& l : layers)
        l.release();
}

void StackVoice::kill()
{
    reset();
}

bool StackVoice::isActive() const
{
    return std::any_of (layers.begin(), layers.end(), [] (const LayerVoice& l) { return l.isActive(); });
}

void StackVoice::render (const StackParams& p, float bendSemis, float vibratoDepth, const float* lfo1, const float* lfo2,
                         float* outL, float* outR, int numSamples)
{
    for (int i = 0; i < numLayers; ++i)
    {
        if (! p.layers[(std::size_t) i].enabled && layers[(std::size_t) i].isActive())
            layers[(std::size_t) i].kill();   // layer switched off mid-note
        else
            layers[(std::size_t) i].setEnvelopes (p.layers[(std::size_t) i]);
    }

    const double glideCoef = p.glideSeconds > 0.0005f ? 1.0 - std::exp (-4.6 / (p.glideSeconds * sampleRate)) : 1.0;
    const float cutoffDepth = p.lfo1ToCutoff * 3.0f;

    for (int s = 0; s < numSamples; ++s)
    {
        currentPitch += (targetPitch - currentPitch) * glideCoef;
        const double pitch = currentPitch + bendSemis + lfo2[s] * vibratoDepth;
        const float cutoffMod = lfo1[s] * cutoffDepth;

        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < numLayers; ++i)
            if (p.layers[(std::size_t) i].enabled)
                layers[(std::size_t) i].processSample (p.layers[(std::size_t) i], pitch, currentPitch, cutoffMod, l, r);

        outL[s] += l;
        outR[s] += r;
    }
}

// ================================================================ engine

void StackEngine::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    maxBlock = std::max (32, maxBlockSize);
    lfo1Buffer.assign ((std::size_t) maxBlock, 0.0f);
    lfo2Buffer.assign ((std::size_t) maxBlock, 0.0f);
    gateBuffer.assign ((std::size_t) maxBlock, 1.0f);
    monoStack.reserve (128);

    std::uint32_t seed = 0x9e3779b9u;
    for (auto& v : voices)
    {
        v.prepare (sr, seed);
        seed = seed * 2654435761u + 12345u;
    }
    gate.prepare (sr);
    lfo1.prepare (sr);
    lfo2.prepare (sr);
    reset();
}

void StackEngine::setParams (const StackParams& p)
{
    const bool modeChanged = p.voiceMode != params.voiceMode;
    params = p;
    arp.setSettings (params.arp);
    if (modeChanged)
        releaseAllVoices();
}

void StackEngine::setSample (int layer, const SampleData* sample)
{
    if (layer < 0 || layer >= numLayers || samples[(std::size_t) layer] == sample)
        return;
    samples[(std::size_t) layer] = sample;
    for (auto& v : voices)
        v.killLayer (layer);
}

void StackEngine::reset()
{
    for (auto& v : voices)
        v.reset();
    arp.reset();
    gate.reset();
    physicallyHeld.fill (false);
    sustainedNote.fill (false);
    sustainDown = false;
    monoStack.clear();
    internalPpq = 0.0;
}

int StackEngine::activeVoiceCount() const
{
    return (int) std::count_if (voices.begin(), voices.end(), [] (const StackVoice& v) { return v.isActive(); });
}

void StackEngine::process (float* outL, float* outR, int numSamples, const InputEvent* events, int numEvents,
                           const Transport& transport)
{
    std::fill (outL, outL + numSamples, 0.0f);
    std::fill (outR, outR + numSamples, 0.0f);

    const double bpm = transport.bpm > 1.0 ? transport.bpm : 120.0;
    beatsPerSample = bpm / 60.0 / sampleRate;
    hostPlaying = transport.playing;
    blockPpq = hostPlaying ? transport.ppqAtBlockStart : internalPpq;

    // Arpeggiator switched on or off since the last block.
    if (params.arp.enabled != arpWasEnabled)
    {
        arpWasEnabled = params.arp.enabled;
        releaseAllVoices();
        arp.reset();
        if (params.arp.enabled)
            for (int n = 0; n < 128; ++n)
                if (physicallyHeld[(std::size_t) n] || sustainedNote[(std::size_t) n])
                    arp.noteOn (n, std::max (0.1f, noteVelocity[(std::size_t) n]));
    }

    int pos = 0;
    for (int i = 0; i < numEvents; ++i)
    {
        const int at = std::clamp (events[i].offset, pos, numSamples);
        segment (outL, outR, pos, at);
        pos = at;
        InputEvent e = events[i];
        e.offset = at;
        handleInput (e);
    }
    segment (outL, outR, pos, numSamples);

    // Trance gate and master volume
    for (int start = 0; start < numSamples; start += maxBlock)
    {
        const int len = std::min (maxBlock, numSamples - start);
        gate.process (params.gate, ppqAt (start), beatsPerSample, gateBuffer.data(), len);
        for (int i = 0; i < len; ++i)
        {
            const float g = gateBuffer[(std::size_t) i] * params.volumeGain;
            outL[start + i] *= g;
            outR[start + i] *= g;
        }
    }

    if (! hostPlaying)
        internalPpq = blockPpq + numSamples * beatsPerSample;
}

void StackEngine::segment (float* outL, float* outR, int start, int end)
{
    if (end <= start)
        return;

    if (! params.arp.enabled)
    {
        renderVoices (outL, outR, start, end);
        return;
    }

    arpEvents.clear();
    arp.generate (ppqAt (start), beatsPerSample, end - start, start, arpEvents);

    int pos = start;
    for (int i = 0; i < arpEvents.size; ++i)
    {
        const auto& e = arpEvents.events[(std::size_t) i];
        const int at = std::clamp (e.offset, pos, end);
        renderVoices (outL, outR, pos, at);
        pos = at;
        if (e.type == NoteEvent::Type::noteOn)
            voiceNoteOn (e.note, e.velocity);
        else
            voiceNoteOff (e.note);
    }
    renderVoices (outL, outR, pos, end);
}

void StackEngine::renderVoices (float* outL, float* outR, int start, int end)
{
    const float bendSemis = bendNormalised * params.bendRangeSemitones;
    const float vibratoDepth = 0.5f * std::min (1.0f, params.lfo2ToPitch + modWheel * params.modWheelToVibrato);

    for (int chunkStart = start; chunkStart < end; chunkStart += maxBlock)
    {
        const int len = std::min (maxBlock, end - chunkStart);
        for (int i = 0; i < len; ++i)
        {
            lfo1Buffer[(std::size_t) i] = lfo1.next (params.lfo1Shape, params.lfo1RateHz);
            lfo2Buffer[(std::size_t) i] = lfo2.next (params.lfo2Shape, params.lfo2RateHz);
        }

        for (auto& v : voices)
            if (v.isActive())
                v.render (params, bendSemis, vibratoDepth, lfo1Buffer.data(), lfo2Buffer.data(),
                          outL + chunkStart, outR + chunkStart, len);
    }
}

// ================================================================ input

void StackEngine::handleInput (const InputEvent& e)
{
    switch (e.type)
    {
        case InputEvent::Type::noteOn:
            if (e.value <= 0.0f)
                noteOffInput (e.note);
            else
                noteOnInput (e.note, e.value, e.offset);
            break;

        case InputEvent::Type::noteOff:
            noteOffInput (e.note);
            break;

        case InputEvent::Type::pitchBend:
            bendNormalised = std::clamp (e.value, -1.0f, 1.0f);
            break;

        case InputEvent::Type::modWheel:
            modWheel = std::clamp (e.value, 0.0f, 1.0f);
            break;

        case InputEvent::Type::sustain:
        {
            const bool down = e.value >= 0.5f;
            if (down == sustainDown)
                break;
            sustainDown = down;
            if (! down)
            {
                for (int n = 0; n < 128; ++n)
                {
                    if (sustainedNote[(std::size_t) n])
                    {
                        sustainedNote[(std::size_t) n] = false;
                        if (! physicallyHeld[(std::size_t) n])
                            routeNoteOff (n);
                    }
                }
            }
            break;
        }

        case InputEvent::Type::allNotesOff:
            physicallyHeld.fill (false);
            sustainedNote.fill (false);
            monoStack.clear();
            arp.reset();
            releaseAllVoices();
            break;

        case InputEvent::Type::allSoundOff:
            reset();
            break;
    }
}

void StackEngine::noteOnInput (int note, float velocity, int offset)
{
    if (note < 0 || note > 127)
        return;

    // Free-running clock: the first note after silence starts the beat grid,
    // so the arpeggiator and gate begin on step one. The arpeggiator's first
    // step waits 10 ms so all the notes of a chord are in before it picks one.
    const bool idle = std::none_of (physicallyHeld.begin(), physicallyHeld.end(), [] (bool b) { return b; })
                   && std::none_of (sustainedNote.begin(), sustainedNote.end(), [] (bool b) { return b; })
                   && ! arp.hasNotes();
    if (idle && ! hostPlaying)
    {
        const double chordWindowSamples = params.arp.enabled ? 0.010 * sampleRate : 0.0;
        blockPpq = -((double) offset + chordWindowSamples) * beatsPerSample;
    }

    physicallyHeld[(std::size_t) note] = true;
    sustainedNote[(std::size_t) note] = false;
    noteVelocity[(std::size_t) note] = velocity;

    if (params.arp.enabled)
        arp.noteOn (note, velocity);
    else
        voiceNoteOn (note, velocity);
}

void StackEngine::noteOffInput (int note)
{
    if (note < 0 || note > 127 || ! physicallyHeld[(std::size_t) note])
        return;
    physicallyHeld[(std::size_t) note] = false;

    if (sustainDown)
    {
        sustainedNote[(std::size_t) note] = true;
        return;
    }
    routeNoteOff (note);
}

void StackEngine::routeNoteOff (int note)
{
    if (params.arp.enabled)
        arp.noteOff (note);
    else
        voiceNoteOff (note);
}

void StackEngine::releaseAllVoices()
{
    for (auto& v : voices)
        if (v.isActive())
            v.release();
    monoStack.clear();
}

void StackEngine::voiceNoteOn (int note, float velocity)
{
    if (params.voiceMode == VoiceMode::mono)
    {
        auto& v = voices[0];
        const bool legato = ! monoStack.empty() && v.isActive() && v.isGateOn();
        monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
        monoStack.push_back (note);
        noteVelocity[(std::size_t) note] = velocity;
        if (legato)
            v.glideTo (note, ++ageCounter);
        else
            v.start (params, samples, note, velocity, glideOn(), ++ageCounter);
        return;
    }

    for (auto& v : voices)
    {
        if (v.isActive() && v.getNote() == note)
        {
            v.start (params, samples, note, velocity, glideOn(), ++ageCounter);
            return;
        }
    }
    findVoiceToUse().start (params, samples, note, velocity, glideOn(), ++ageCounter);
}

void StackEngine::voiceNoteOff (int note)
{
    if (params.voiceMode == VoiceMode::mono)
    {
        auto& v = voices[0];
        monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
        if (monoStack.empty())
        {
            if (v.isGateOn() && v.getNote() == note)
                v.release();
        }
        else if (monoStack.back() != v.getNote())
        {
            v.glideTo (monoStack.back(), ++ageCounter);
        }
        return;
    }

    for (auto& v : voices)
        if (v.isGateOn() && v.getNote() == note)
            v.release();
}

StackVoice& StackEngine::findVoiceToUse()
{
    for (auto& v : voices)
        if (! v.isActive())
            return v;

    StackVoice* best = nullptr;
    for (auto& v : voices)
        if (! v.isGateOn() && (best == nullptr || v.getAge() < best->getAge()))
            best = &v;
    if (best == nullptr)
        for (auto& v : voices)
            if (best == nullptr || v.getAge() < best->getAge())
                best = &v;
    return *best;
}

} // namespace stack
