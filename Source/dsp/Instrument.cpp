#include "Instrument.h"
#include <algorithm>
#include <cmath>

namespace ladder
{

void Instrument::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    chunkSize = std::max (32, std::min (maxBlockSize, 1024));
    mono.assign ((size_t) chunkSize, 0.0f);

    engine.prepare (sampleRate);
    gate.prepare (sampleRate);
    chorus.prepare (sampleRate);
    delay.prepare (sampleRate);
    reverb.prepare (sampleRate);
    reset();
}

void Instrument::setParams (const SynthParams& p)
{
    params = p;
    engine.setParams (p);
    arp.setSettings (p.arp, beat);

    if (p.arp.enabled != arpWasOn)
    {
        arpWasOn = p.arp.enabled;
        if (p.arp.enabled)
        {
            // Hand the keys that are down over to the arpeggiator.
            engine.allNotesOff();
            arp.clear (*this);
            for (int n = 0; n < 128; ++n)
                if (keyDown[(size_t) n])
                    arp.noteOn (n, keyVelocity[(size_t) n]);
            arp.setSustain (sustainDown);
            arp.resync (beat);
        }
        else
        {
            // Back to playing the keys directly.
            arp.clear (*this);
            engine.allNotesOff();
            for (int n = 0; n < 128; ++n)
                if (keyDown[(size_t) n])
                    engine.noteOn (n, keyVelocity[(size_t) n]);
        }
    }

    // Clear an effect's memory when it is switched on, so no stale tail bursts out.
    if (p.chorus.enabled && ! chorusWasOn) chorus.reset();
    if (p.delay.enabled && ! delayWasOn) delay.reset();
    if (p.reverb.enabled && ! reverbWasOn) reverb.reset();
    chorusWasOn = p.chorus.enabled;
    delayWasOn = p.delay.enabled;
    reverbWasOn = p.reverb.enabled;
}

void Instrument::setTransport (double newBpm, bool playing, double ppq)
{
    if (std::isfinite (newBpm) && newBpm > 1.0)
        bpm = std::clamp (newBpm, 20.0, 999.0);

    if (playing && std::isfinite (ppq))
    {
        // Follow the host. A jump (loop, relocate, start) re-aligns the arpeggiator.
        const bool jumped = ! hostPlaying || std::fabs (ppq - beat) > 0.05;
        beat = ppq;
        if (jumped)
        {
            arp.silence (*this);
            arp.resync (beat);
        }
    }
    hostPlaying = playing;
}

void Instrument::restartFreeClock()
{
    arp.silence (*this); // their note-offs were scheduled on the old timeline
    beat = 0.0;
    arp.resync (beat);
}

void Instrument::noteOn (int note, float velocity)
{
    if (note < 0 || note > 127)
        return;

    // A fresh phrase (no keys down, nothing latched by the pedal) restarts a free-running clock.
    const bool freshPhrase = params.arp.enabled ? keysDown == 0 && (! arp.hasHeldNotes() || params.arp.hold)
                                                : keysDown == 0 && ! sustainDown;
    if (freshPhrase && ! hostPlaying)
        restartFreeClock();

    if (! keyDown[(size_t) note])
        ++keysDown;
    keyDown[(size_t) note] = true;
    keyVelocity[(size_t) note] = velocity;

    if (params.arp.enabled)
        arp.noteOn (note, velocity);
    else
        engine.noteOn (note, velocity);
}

void Instrument::noteOff (int note)
{
    if (note < 0 || note > 127)
        return;

    if (keyDown[(size_t) note])
        --keysDown;
    keyDown[(size_t) note] = false;

    if (params.arp.enabled)
        arp.noteOff (note);
    else
        engine.noteOff (note);
}

void Instrument::setSustainPedal (bool down)
{
    sustainDown = down;
    arp.setSustain (down);
    engine.setSustainPedal (down);
}

void Instrument::allNotesOff()
{
    arp.clear (*this);
    engine.allNotesOff();
    keyDown.fill (false);
    keysDown = 0;
}

void Instrument::reset()
{
    arp.clear (*this);
    engine.reset();
    gate.reset();
    chorus.reset();
    delay.reset();
    reverb.reset();
    keyDown.fill (false);
    keysDown = 0;
    sustainDown = false;
    arp.setSustain (false);
    beat = 0.0;
    arp.resync (beat);
}

int Instrument::currentGateStep() const
{
    if (! params.gate.enabled)
        return -1;
    return TranceGate::stepAt (params.gate, beat);
}

void Instrument::render (float* left, float* right, int numSamples)
{
    int done = 0;
    while (done < numSamples)
    {
        int n = std::min (numSamples - done, chunkSize);

        if (params.arp.enabled)
        {
            // Stop exactly where the arpeggiator has something to do.
            const double bps = beatsPerSample();
            const double halfSample = 0.5 * bps;
            const double next = arp.nextEventBeat();
            if (next < Arpeggiator::never)
            {
                const double samplesAway = std::ceil ((next - halfSample - beat) / bps);
                if (samplesAway <= 0.0)
                {
                    arp.processUntil (beat + halfSample, *this);
                    continue;
                }
                if (samplesAway < (double) n)
                    n = (int) samplesAway;
            }
        }

        renderSegment (left + done, right + done, n);
        done += n;
    }
}

void Instrument::renderSegment (float* left, float* right, int numSamples)
{
    const double bps = beatsPerSample();

    engine.render (mono.data(), numSamples);
    gate.process (params.gate, mono.data(), numSamples, beat, bps);
    chorus.process (params.chorus, mono.data(), left, right, numSamples);
    delay.process (params.delay, bpm, left, right, numSamples);
    reverb.process (params.reverb, left, right, numSamples);

    beat += bps * numSamples;
}

} // namespace ladder
