#pragma once

#include "SynthParams.h"
#include <cmath>

namespace ladder
{

// Band-limited oscillator using PolyBLEP corrections on every hard edge.
class Oscillator
{
public:
    void reset (double startPhase = 0.0) { phase = startPhase; }

    // frequencyHz may change every sample; returns a sample in roughly -1..1.
    float process (Waveform wave, double frequencyHz, double sampleRate)
    {
        const double dt = std::fmin (std::fabs (frequencyHz) / sampleRate, 0.49);
        float out = 0.0f;

        switch (wave)
        {
            case Waveform::triangle:    out = triangle(); break;
            case Waveform::triSaw:      out = 0.5f * (triangle() + saw (dt)); break;
            case Waveform::reverseSaw:  out = -saw (dt); break;
            case Waveform::saw:         out = saw (dt); break;
            case Waveform::square:      out = pulse (0.5, dt); break;
            case Waveform::widePulse:   out = pulse (0.3, dt) * 1.1f; break;
            case Waveform::narrowPulse: out = pulse (0.12, dt) * 1.3f; break;
        }

        phase += dt;
        if (phase >= 1.0)
            phase -= 1.0;

        return out;
    }

private:
    double phase = 0.0;

    static double polyBlep (double t, double dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0;
        }
        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }
        return 0.0;
    }

    float saw (double dt) const
    {
        return (float) (2.0 * phase - 1.0 - polyBlep (phase, dt));
    }

    float pulse (double width, double dt) const
    {
        double value = phase < width ? 1.0 : -1.0;
        value += polyBlep (phase, dt);
        double shifted = phase - width;
        if (shifted < 0.0)
            shifted += 1.0;
        value -= polyBlep (shifted, dt);
        // Remove the DC offset of asymmetric pulses.
        return (float) (value - (2.0 * width - 1.0));
    }

    float triangle() const
    {
        // Triangle has no hard edges, so the naive form aliases very little.
        return (float) (1.0 - 4.0 * std::fabs (phase - 0.5));
    }
};

} // namespace ladder
