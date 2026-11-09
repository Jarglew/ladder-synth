#pragma once

#include "SynthParams.h"
#include <algorithm>
#include <cmath>

namespace ladder
{

// Analog-style ADSR: each segment is an exponential approach to a target.
// The attack aims past 1.0 so it reaches the top in the set time with the
// familiar convex curve.
class Envelope
{
public:
    enum class Stage { idle, attack, decay, sustain, release };

    void prepare (double newSampleRate) { sampleRate = newSampleRate; reset(); }
    void reset() { stage = Stage::idle; level = 0.0f; }

    void setSettings (const EnvelopeSettings& s) { settings = s; }

    void noteOn() { stage = Stage::attack; }   // retriggers from the current level
    void noteOff()
    {
        if (stage != Stage::idle)
            stage = Stage::release;
    }

    bool isActive() const { return stage != Stage::idle; }
    Stage getStage() const { return stage; }

    float process()
    {
        switch (stage)
        {
            case Stage::idle:
                return 0.0f;

            case Stage::attack:
            {
                // Aiming at 1.3, the curve crosses 1.0 after ln(1.3 / 0.3) = 1.466 time constants.
                const float overshoot = 1.3f;
                level += (overshoot - level) * coefficient (settings.attack, 1.466f);
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    stage = Stage::decay;
                }
                break;
            }

            case Stage::decay:
                level += (settings.sustain - level) * coefficient (settings.decay, 4.6f);
                if (std::fabs (level - settings.sustain) < 1.0e-4f)
                {
                    level = settings.sustain;
                    stage = Stage::sustain;
                }
                break;

            case Stage::sustain:
                // Follows sustain changes smoothly while the key is held.
                level += (settings.sustain - level) * 0.001f;
                break;

            case Stage::release:
                level += (0.0f - level) * coefficient (settings.release, 4.6f);
                if (level < 1.0e-5f)
                {
                    level = 0.0f;
                    stage = Stage::idle;
                }
                break;
        }

        return level;
    }

private:
    EnvelopeSettings settings;
    Stage stage = Stage::idle;
    float level = 0.0f;
    double sampleRate = 44100.0;

    // timeConstants: how many time constants fit in the segment time.
    float coefficient (float seconds, float timeConstants) const
    {
        const double samples = std::max (1.0, (double) seconds * sampleRate);
        return (float) (1.0 - std::exp (-timeConstants / samples));
    }
};

} // namespace ladder
