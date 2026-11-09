#pragma once

#include <algorithm>
#include <cmath>

namespace stack
{

// 12 dB/oct state-variable filter (topology-preserving transform), giving
// low-pass, high-pass and band-pass from one structure. Stable under fast
// cutoff modulation.
class Svf
{
public:
    enum class Mode { lowPass, highPass, bandPass };

    struct Coefficients
    {
        float g = 0.0f, k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    };

    static Coefficients makeCoefficients (float cutoffHz, float resonance, double sampleRate)
    {
        const float fc = std::clamp (cutoffHz, 10.0f, (float) (0.45 * sampleRate));
        Coefficients c;
        c.g = (float) std::tan (3.14159265358979 * fc / sampleRate);
        // resonance 0..1 -> damping 2..0.05 (Q 0.5..20)
        c.k = 2.0f - 1.95f * std::clamp (resonance, 0.0f, 1.0f);
        c.a1 = 1.0f / (1.0f + c.g * (c.g + c.k));
        c.a2 = c.g * c.a1;
        c.a3 = c.g * c.a2;
        return c;
    }

    void reset() { ic1 = ic2 = 0.0f; }

    float process (float v0, const Coefficients& c, Mode mode)
    {
        const float v3 = v0 - ic2;
        const float v1 = c.a1 * ic1 + c.a2 * v3;
        const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;

        switch (mode)
        {
            case Mode::lowPass:  return v2;
            case Mode::bandPass: return v1;
            case Mode::highPass: return v0 - c.k * v1 - v2;
        }
        return v2;
    }

private:
    float ic1 = 0.0f, ic2 = 0.0f;
};

} // namespace stack
