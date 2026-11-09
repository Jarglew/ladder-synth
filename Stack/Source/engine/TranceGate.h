#pragma once

#include "StackParams.h"
#include <cmath>
#include <cstdint>

namespace stack
{

// Rhythmic volume gate: a 16-step on/off pattern locked to the beat grid,
// with attack/release smoothing so the chops don't click.
class TranceGate
{
public:
    void prepare (double newSampleRate) { sampleRate = newSampleRate; gain = 1.0f; }
    void reset() { gain = 1.0f; }

    // Writes one gain value per sample into gains.
    void process (const GateSettings& s, double ppqStart, double beatsPerSample, float* gains, int numSamples)
    {
        const float attackCoef = coefficient (s.attackMs);
        const float releaseCoef = coefficient (s.releaseMs);
        const double stepLen = rateInBeats (s.rate);
        const int steps = s.steps < 1 ? 1 : (s.steps > maxSteps ? maxSteps : s.steps);

        for (int i = 0; i < numSamples; ++i)
        {
            float target = 1.0f;
            if (s.enabled)
            {
                const double ppq = ppqStart + i * beatsPerSample;
                const auto step = (std::int64_t) std::floor (ppq / stepLen);
                const auto index = (std::size_t) (((step % steps) + steps) % steps);
                target = s.stepOn[index] ? 1.0f : 1.0f - s.depth;
            }
            gain += (target - gain) * (target > gain ? attackCoef : releaseCoef);
            gains[i] = gain;
        }
    }

private:
    double sampleRate = 44100.0;
    float gain = 1.0f;

    float coefficient (float ms) const
    {
        const double samples = std::fmax (1.0, ms * 0.001 * sampleRate);
        return (float) (1.0 - std::exp (-2.2 / samples)); // ~90% of the way in the given time
    }
};

// Free-running low-frequency oscillator, output -1..1.
class Lfo
{
public:
    void prepare (double newSampleRate) { sampleRate = newSampleRate; phase = 0.0; }

    float next (LfoShape shape, float rateHz)
    {
        float out = 0.0f;
        switch (shape)
        {
            case LfoShape::sine:       out = (float) std::sin (2.0 * 3.14159265358979 * phase); break;
            case LfoShape::triangle:   out = (float) (1.0 - 4.0 * std::fabs (phase - 0.5)); break;
            case LfoShape::saw:        out = (float) (2.0 * phase - 1.0); break;
            case LfoShape::square:     out = phase < 0.5 ? 1.0f : -1.0f; break;
            case LfoShape::sampleHold: out = held; break;
        }

        phase += rateHz / sampleRate;
        if (phase >= 1.0)
        {
            phase -= 1.0;
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            held = (float) seed / 2147483648.0f - 1.0f;
        }
        return out;
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    std::uint32_t seed = 0x1b873593u;
    float held = 0.0f;
};

} // namespace stack
