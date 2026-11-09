#pragma once

#include "Envelope.h"
#include "LadderFilter.h"
#include "Oscillator.h"
#include "SynthParams.h"
#include <cmath>
#include <cstdint>

namespace ladder
{

// Performance state shared by every voice.
struct PerformanceState
{
    float bendSemitones = 0.0f;
    float modWheel = 0.0f;   // 0..1
};

class Voice
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        filter.prepare (sampleRate);
        ampEnv.prepare (sampleRate);
        filterEnv.prepare (sampleRate);
        reset();
    }

    void reset()
    {
        filter.reset();
        ampEnv.reset();
        filterEnv.reset();
        gate = false;
        sustained = false;
        hasPlayed = false;
        note = -1;
    }

    // Starts a note. With glide on, the pitch slides from wherever this voice was.
    void start (int midiNote, float newVelocity, bool glide, std::uint64_t newAge)
    {
        if (! hasPlayed || ! glide)
            currentPitch = (double) midiNote;

        if (! ampEnv.isActive())
        {
            filter.reset();
            // Free-running oscillators, but start from spread phases for a fuller attack.
            osc[0].reset (0.0);
            osc[1].reset (0.33);
            osc[2].reset (0.66);
        }

        note = midiNote;
        targetPitch = (double) midiNote;
        velocity = newVelocity;
        gate = true;
        sustained = false;
        hasPlayed = true;
        age = newAge;
        ampEnv.noteOn();
        filterEnv.noteOn();
    }

    // Moves to a new note without retriggering the envelopes (legato).
    void glideTo (int midiNote, std::uint64_t newAge)
    {
        note = midiNote;
        targetPitch = (double) midiNote;
        gate = true;
        age = newAge;
    }

    void release()
    {
        gate = false;
        sustained = false;
        ampEnv.noteOff();
        filterEnv.noteOff();
    }

    void seedNoise (std::uint32_t seed) { noiseSeed = seed | 1u; }

    bool isActive() const { return ampEnv.isActive(); }
    bool isGateOn() const { return gate; }
    bool isSustained() const { return sustained; }
    void setSustained (bool s) { sustained = s; }
    int getNote() const { return note; }
    std::uint64_t getAge() const { return age; }

    void render (const SynthParams& p, const PerformanceState& perf, float* out, int numSamples)
    {
        if (! isActive())
            return;

        ampEnv.setSettings (p.ampEnv);
        filterEnv.setSettings (p.filterEnv);

        const double glideCoef = p.glideSeconds > 0.0005f
            ? 1.0 - std::exp (-4.6 / (p.glideSeconds * sampleRate)) // arrives (99%) in glideSeconds
            : 1.0;

        const float modDepth = std::fmin (1.0f, p.modAmount + perf.modWheel);
        const float modDepthCurved = modDepth * modDepth;
        const float keyTrackAmount = (float) p.keyTrack / 3.0f;
        const float tuneSemis = p.masterTuneCents / 100.0f;
        const float driveGain = 1.0f + 7.0f * p.drive * p.drive;
        const float targetCutoffOct = std::log2 (std::fmax (p.cutoffHz, 10.0f));
        const float ampVelocityGain = 1.0f - p.velocityToAmp + p.velocityToAmp * velocity;

        if (cutoffOct < 0.0f)
            cutoffOct = targetCutoffOct;

        for (int i = 0; i < numSamples; ++i)
        {
            currentPitch += (targetPitch - currentPitch) * glideCoef;

            const float noise = nextNoise (p.noiseColour);

            // Oscillator 3 runs first so it can act as the modulation source.
            const double osc3Base = p.osc3KeyTrack ? currentPitch + perf.bendSemitones : 60.0;
            const double osc3Pitch = osc3Base + rangeSemitones (p.osc[2].range) + p.osc[2].tuneSemitones + tuneSemis;
            const float o3 = osc[2].process (p.osc[2].wave, pitchToHz (osc3Pitch), sampleRate);

            const float modSignal = (1.0f - p.modMix) * o3 + p.modMix * noise;
            const float pitchMod = p.oscMod ? modSignal * modDepthCurved * 2.0f : 0.0f;

            const double keyedPitch = currentPitch + perf.bendSemitones + tuneSemis + pitchMod;
            const float o1 = osc[0].process (p.osc[0].wave, pitchToHz (keyedPitch + rangeSemitones (p.osc[0].range)), sampleRate);
            const float o2 = osc[1].process (p.osc[1].wave,
                                             pitchToHz (keyedPitch + rangeSemitones (p.osc[1].range) + p.osc[1].tuneSemitones),
                                             sampleRate);

            const float mix = 0.35f * (p.mixOsc[0] * o1 + p.mixOsc[1] * o2 + p.mixOsc[2] * o3 + p.mixNoise * noise);

            const float fEnv = filterEnv.process();
            const float aEnv = ampEnv.process();

            cutoffOct += (targetCutoffOct - cutoffOct) * 0.002f; // de-zippers knob moves
            float octaves = cutoffOct
                          + p.contour * 6.0f * fEnv
                          + keyTrackAmount * (float) (currentPitch - 60.0) / 12.0f
                          + p.velocityToFilter * velocity * 3.0f;
            if (p.filterMod)
                octaves += modSignal * modDepthCurved * 2.0f;

            const float filtered = filter.process (mix, std::exp2 (octaves), p.emphasis, driveGain);
            out[i] += filtered * aEnv * ampVelocityGain;

            if (! ampEnv.isActive())
                break;
        }
    }

private:
    double sampleRate = 44100.0;
    Oscillator osc[3];
    LadderFilter filter;
    Envelope ampEnv, filterEnv;

    int note = -1;
    float velocity = 1.0f;
    bool gate = false;
    bool sustained = false;
    bool hasPlayed = false;
    std::uint64_t age = 0;
    double currentPitch = 60.0;
    double targetPitch = 60.0;
    float cutoffOct = -1.0f;

    std::uint32_t noiseSeed = 0x9e3779b9u;
    float pink[3] {};

    static double pitchToHz (double midiPitch) { return 440.0 * std::exp2 ((midiPitch - 69.0) / 12.0); }

    static double rangeSemitones (Range r)
    {
        switch (r)
        {
            case Range::lo:   return -72.0;
            case Range::ft32: return -24.0;
            case Range::ft16: return -12.0;
            case Range::ft8:  return 0.0;
            case Range::ft4:  return 12.0;
            case Range::ft2:  return 24.0;
        }
        return 0.0;
    }

    float nextNoise (NoiseColour colour)
    {
        // xorshift32 white noise in -1..1
        noiseSeed ^= noiseSeed << 13;
        noiseSeed ^= noiseSeed >> 17;
        noiseSeed ^= noiseSeed << 5;
        const float white = (float) noiseSeed / 2147483648.0f - 1.0f;
        if (colour == NoiseColour::white)
            return white;

        // Paul Kellet's economy pink filter
        pink[0] = 0.99765f * pink[0] + white * 0.0990460f;
        pink[1] = 0.96300f * pink[1] + white * 0.2965164f;
        pink[2] = 0.57000f * pink[2] + white * 1.0526913f;
        return 0.25f * (pink[0] + pink[1] + pink[2] + white * 0.1848f);
    }
};

} // namespace ladder
