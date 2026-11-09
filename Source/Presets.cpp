#include "Presets.h"
#include "Parameters.h"

namespace
{
// Choice indices, to keep the tables readable.
namespace R { constexpr float lo = 0, ft32 = 1, ft16 = 2, ft8 = 3, ft4 = 4, ft2 = 5; }
namespace W { constexpr float tri = 0, triSaw = 1, revSaw = 2, saw = 3, square = 4, wide = 5, narrow = 6; }
constexpr float mono = 0, poly = 1;
} // namespace

const std::vector<Preset>& factoryPresets()
{
    using namespace ParamIDs;

    static const std::vector<Preset> presets {
        { "Init", {} },

        { "Fat Bass",
          { { voiceMode, mono }, { osc1Range, R::ft16 }, { osc1Wave, W::saw },
            { osc2Range, R::ft16 }, { osc2Wave, W::saw }, { osc2Tune, 0.07f },
            { osc3Range, R::ft32 }, { osc3Wave, W::square },
            { mixOsc1, 1.0f }, { mixOsc2, 0.9f }, { mixOsc3, 0.6f },
            { cutoff, 260.0f }, { emphasis, 0.35f }, { contour, 0.55f }, { drive, 0.35f },
            { fAttack, 0.001f }, { fDecay, 0.35f }, { fSustain, 0.15f }, { fRelease, 0.2f },
            { aAttack, 0.001f }, { aDecay, 0.5f }, { aSustain, 0.9f }, { aRelease, 0.12f },
            { glide, 0.04f } } },

        { "Sub Rumble",
          { { voiceMode, mono }, { osc1Range, R::ft32 }, { osc1Wave, W::tri },
            { osc2Range, R::ft16 }, { osc2Wave, W::triSaw }, { mixOsc2, 0.5f },
            { cutoff, 180.0f }, { emphasis, 0.1f }, { contour, 0.25f }, { drive, 0.5f },
            { fDecay, 0.6f }, { fSustain, 0.4f },
            { aAttack, 0.005f }, { aSustain, 1.0f }, { aRelease, 0.2f } } },

        { "Classic Lead",
          { { voiceMode, mono }, { osc1Wave, W::saw }, { osc2Wave, W::square }, { osc2Tune, 0.1f },
            { osc3Range, R::lo }, { osc3Wave, W::tri }, { osc3KeyTrack, 0.0f },
            { mixOsc1, 1.0f }, { mixOsc2, 0.8f },
            { cutoff, 1200.0f }, { emphasis, 0.45f }, { contour, 0.4f }, { keyTrack, 2.0f },
            { fAttack, 0.01f }, { fDecay, 0.5f }, { fSustain, 0.5f },
            { aSustain, 1.0f }, { aRelease, 0.15f },
            { glide, 0.12f }, { oscMod, 1.0f }, { modMix, 0.0f } } },

        { "Squelch Lead",
          { { voiceMode, mono }, { osc1Wave, W::square }, { osc2Wave, W::narrow }, { osc2Range, R::ft4 },
            { mixOsc2, 0.5f }, { cutoff, 350.0f }, { emphasis, 0.8f }, { contour, 0.7f },
            { fAttack, 0.003f }, { fDecay, 0.25f }, { fSustain, 0.2f }, { drive, 0.5f },
            { aSustain, 0.9f }, { glide, 0.08f }, { legato, 1.0f } } },

        { "Brass",
          { { voiceMode, poly }, { osc1Wave, W::saw }, { osc2Wave, W::saw }, { osc2Tune, -0.08f },
            { mixOsc2, 0.9f }, { cutoff, 450.0f }, { emphasis, 0.15f }, { contour, 0.7f },
            { fAttack, 0.08f }, { fDecay, 0.6f }, { fSustain, 0.4f }, { fRelease, 0.3f },
            { aAttack, 0.05f }, { aDecay, 0.4f }, { aSustain, 0.9f }, { aRelease, 0.3f },
            { velFilter, 0.3f } } },

        { "Pluck",
          { { voiceMode, poly }, { osc1Wave, W::wide }, { osc2Wave, W::saw }, { osc2Range, R::ft4 },
            { mixOsc2, 0.4f }, { cutoff, 380.0f }, { emphasis, 0.5f }, { contour, 0.8f },
            { fAttack, 0.001f }, { fDecay, 0.25f }, { fSustain, 0.0f }, { fRelease, 0.25f },
            { aAttack, 0.001f }, { aDecay, 0.7f }, { aSustain, 0.0f }, { aRelease, 0.35f },
            { velFilter, 0.4f } } },

        { "Soft Pad",
          { { voiceMode, poly }, { osc1Wave, W::triSaw }, { osc2Wave, W::tri }, { osc2Tune, 0.12f },
            { osc3Wave, W::saw }, { osc3Range, R::ft16 }, { mixOsc2, 0.8f }, { mixOsc3, 0.35f },
            { cutoff, 900.0f }, { emphasis, 0.2f }, { contour, 0.3f },
            { fAttack, 1.5f }, { fDecay, 2.0f }, { fSustain, 0.5f }, { fRelease, 1.5f },
            { aAttack, 1.2f }, { aDecay, 1.0f }, { aSustain, 1.0f }, { aRelease, 1.6f },
            { velAmp, 0.2f } } },

        { "Laser Zap",
          { { voiceMode, mono }, { osc1Wave, W::square }, { cutoff, 90.0f }, { emphasis, 0.88f },
            { contour, 1.0f }, { fAttack, 0.001f }, { fDecay, 0.18f }, { fSustain, 0.0f },
            { aDecay, 0.4f }, { aSustain, 0.0f }, { aRelease, 0.3f }, { legato, 0.0f } } },

        { "Filter Wobble",
          { { voiceMode, mono }, { osc1Wave, W::saw }, { osc1Range, R::ft16 }, { osc2Wave, W::saw },
            { osc2Range, R::ft16 }, { osc2Tune, 0.05f }, { mixOsc2, 0.8f },
            { osc3Range, R::lo }, { osc3Wave, W::tri }, { osc3Tune, -3.0f }, { osc3KeyTrack, 0.0f },
            { cutoff, 400.0f }, { emphasis, 0.55f }, { contour, 0.2f }, { drive, 0.3f },
            { filterMod, 1.0f }, { modAmount, 0.7f }, { aSustain, 1.0f } } },
    };

    return presets;
}

void applyPreset (const Preset& preset, juce::AudioProcessorValueTreeState& apvts)
{
    for (auto* param : apvts.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());

    for (const auto& [id, value] : preset.values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
}
