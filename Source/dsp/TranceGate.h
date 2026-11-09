#pragma once

#include "SynthParams.h"
#include <cmath>

namespace ladder
{

// 16-step rhythmic volume gate, locked to the beat clock.
// Each open step opens for `length` of the step, so runs of open steps still pulse.
class TranceGate
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }

    void reset() { gain = 1.0f; }

    // Applies the gate in place. beat is the clock at the first sample.
    void process (const GateSettings& s, float* buffer, int numSamples, double beat, double beatsPerSample)
    {
        const float smoothSeconds = 0.0003f + 0.025f * s.smooth * s.smooth;
        const float coef = 1.0f - (float) std::exp (-1.0 / (smoothSeconds * sampleRate));

        if (! s.enabled)
        {
            if (gain > 0.9999f)
                return;
            for (int i = 0; i < numSamples; ++i)
            {
                gain += (1.0f - gain) * coef;
                buffer[i] *= gain;
            }
            return;
        }

        const double stepLen = noteLengthBeats (s.rate);
        const float closed = 1.0f - s.depth;

        for (int i = 0; i < numSamples; ++i)
        {
            const float target = isOpen (s, beat + (double) i * beatsPerSample, stepLen) ? 1.0f : closed;
            gain += (target - gain) * coef;
            buffer[i] *= gain;
        }
    }

    static int stepAt (const GateSettings& s, double beat)
    {
        const double steps = std::floor (beat / noteLengthBeats (s.rate));
        const long long n = (long long) steps % GateSettings::numSteps;
        return (int) (n < 0 ? n + GateSettings::numSteps : n);
    }

private:
    double sampleRate = 44100.0;
    float gain = 1.0f;

    static bool isOpen (const GateSettings& s, double beat, double stepLen)
    {
        const double pos = beat / stepLen;
        const double whole = std::floor (pos);
        const long long n = (long long) whole % GateSettings::numSteps;
        const int step = (int) (n < 0 ? n + GateSettings::numSteps : n);
        return s.steps[step] && (pos - whole) < (double) s.length;
    }
};

} // namespace ladder
