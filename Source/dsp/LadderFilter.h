#pragma once

#include <algorithm>
#include <cmath>

namespace ladder
{

// Four-pole (24 dB/oct) transistor-ladder style low-pass filter.
// Zero-delay-feedback (topology-preserving transform) structure with a tanh
// saturator at the input stage, so heavy resonance and drive stay bounded and
// the filter self-oscillates at full emphasis.
class LadderFilter
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }

    void reset()
    {
        for (auto& s : state)
            s = 0.0f;
    }

    // resonance 0..1 maps onto feedback 0..~4.2 (self-oscillation from ~0.95).
    float process (float input, float cutoffHz, float resonance, float driveGain)
    {
        const float fc = std::clamp (cutoffHz, 10.0f, (float) (0.45 * sampleRate));
        const float g = (float) std::tan (3.14159265358979 * fc / sampleRate);
        const float G = g / (1.0f + g);
        const float k = 4.2f * std::clamp (resonance, 0.0f, 1.0f);

        // Each stage's state contribution, weighted by how much of it reaches the output.
        const float sigma = (G * G * G * state[0] + G * G * state[1] + G * state[2] + state[3]) / (1.0f + g);

        // Partial passband gain compensation keeps resonant patches from going thin.
        const float x = input * driveGain * (1.0f + 0.35f * k);
        float u = (x - k * sigma) / (1.0f + k * G * G * G * G);
        u = std::tanh (u);

        for (auto& s : state)
        {
            const float v = (u - s) * G;
            const float y = v + s;
            s = y + v;
            u = y;
        }

        return u;
    }

private:
    double sampleRate = 44100.0;
    float state[4] {};
};

} // namespace ladder
