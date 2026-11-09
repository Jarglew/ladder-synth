#include "Presets.h"
#include "Parameters.h"

namespace
{
// Choice indices, to keep the tables readable.
namespace R { constexpr float lo = 0, ft32 = 1, ft16 = 2, ft8 = 3, ft4 = 4, ft2 = 5; }
namespace W { constexpr float tri = 0, triSaw = 1, revSaw = 2, saw = 3, square = 4, wide = 5, narrow = 6; }
constexpr float mono = 0, poly = 1;

// Choice indices for the newer parameters (see the tables in Parameters.cpp).
namespace Arp { constexpr float up = 0, down = 1, upDown = 2, random = 3, asPlayed = 4, chord = 5; }
namespace ArpRate { constexpr float q = 0, e_dot = 2, e = 3, e_tri = 4, s = 6, s_tri = 7, t = 8; }
namespace GateRate { constexpr float e = 0, e_tri = 1, s = 2, s_tri = 3, t = 4; }
namespace DelayTime { constexpr float h = 0, q_dot = 1, q = 2, e_dot = 4, e = 5, s = 8; }

using Values = std::vector<std::pair<const char*, float>>;

// Appends a gate pattern written as 16 characters: 'x' open, '.' closed.
Values gatePattern (Values v, const char* pattern)
{
    for (int i = 0; i < 16 && pattern[i] != 0; ++i)
        v.push_back ({ ParamIDs::gateSteps[i], pattern[i] == 'x' ? 1.0f : 0.0f });
    return v;
}
} // namespace

const std::vector<const char*>& presetCategories()
{
    static const std::vector<const char*> categories { "Init", "Bass", "Lead", "Keys", "Pluck", "Pad", "Arp", "Gated", "FX" };
    return categories;
}

const std::vector<Preset>& factoryPresets()
{
    using namespace ParamIDs;

    static const std::vector<Preset> presets {
        { "Init", "Init", {} },

        { "Bass", "Fat Bass",
          { { voiceMode, mono }, { osc1Range, R::ft16 }, { osc1Wave, W::saw },
            { osc2Range, R::ft16 }, { osc2Wave, W::saw }, { osc2Tune, 0.07f },
            { osc3Range, R::ft32 }, { osc3Wave, W::square },
            { mixOsc1, 1.0f }, { mixOsc2, 0.9f }, { mixOsc3, 0.6f },
            { cutoff, 260.0f }, { emphasis, 0.35f }, { contour, 0.55f }, { drive, 0.35f },
            { fAttack, 0.001f }, { fDecay, 0.35f }, { fSustain, 0.15f }, { fRelease, 0.2f },
            { aAttack, 0.001f }, { aDecay, 0.5f }, { aSustain, 0.9f }, { aRelease, 0.12f },
            { glide, 0.04f } } },

        { "Bass", "Sub Rumble",
          { { voiceMode, mono }, { osc1Range, R::ft32 }, { osc1Wave, W::tri },
            { osc2Range, R::ft16 }, { osc2Wave, W::triSaw }, { mixOsc2, 0.5f },
            { cutoff, 180.0f }, { emphasis, 0.1f }, { contour, 0.25f }, { drive, 0.5f },
            { fDecay, 0.6f }, { fSustain, 0.4f },
            { aAttack, 0.005f }, { aSustain, 1.0f }, { aRelease, 0.2f } } },

        { "Lead", "Classic Lead",
          { { voiceMode, mono }, { osc1Wave, W::saw }, { osc2Wave, W::square }, { osc2Tune, 0.1f },
            { osc3Range, R::lo }, { osc3Wave, W::tri }, { osc3KeyTrack, 0.0f },
            { mixOsc1, 1.0f }, { mixOsc2, 0.8f },
            { cutoff, 1200.0f }, { emphasis, 0.45f }, { contour, 0.4f }, { keyTrack, 2.0f },
            { fAttack, 0.01f }, { fDecay, 0.5f }, { fSustain, 0.5f },
            { aSustain, 1.0f }, { aRelease, 0.15f },
            { glide, 0.12f }, { oscMod, 1.0f }, { modMix, 0.0f } } },

        { "Lead", "Squelch Lead",
          { { voiceMode, mono }, { osc1Wave, W::square }, { osc2Wave, W::narrow }, { osc2Range, R::ft4 },
            { mixOsc2, 0.5f }, { cutoff, 350.0f }, { emphasis, 0.8f }, { contour, 0.7f },
            { fAttack, 0.003f }, { fDecay, 0.25f }, { fSustain, 0.2f }, { drive, 0.5f },
            { aSustain, 0.9f }, { glide, 0.08f }, { legato, 1.0f } } },

        { "Keys", "Brass",
          { { voiceMode, poly }, { osc1Wave, W::saw }, { osc2Wave, W::saw }, { osc2Tune, -0.08f },
            { mixOsc2, 0.9f }, { cutoff, 450.0f }, { emphasis, 0.15f }, { contour, 0.7f },
            { fAttack, 0.08f }, { fDecay, 0.6f }, { fSustain, 0.4f }, { fRelease, 0.3f },
            { aAttack, 0.05f }, { aDecay, 0.4f }, { aSustain, 0.9f }, { aRelease, 0.3f },
            { velFilter, 0.3f } } },

        { "Pluck", "Pluck",
          { { voiceMode, poly }, { osc1Wave, W::wide }, { osc2Wave, W::saw }, { osc2Range, R::ft4 },
            { mixOsc2, 0.4f }, { cutoff, 380.0f }, { emphasis, 0.5f }, { contour, 0.8f },
            { fAttack, 0.001f }, { fDecay, 0.25f }, { fSustain, 0.0f }, { fRelease, 0.25f },
            { aAttack, 0.001f }, { aDecay, 0.7f }, { aSustain, 0.0f }, { aRelease, 0.35f },
            { velFilter, 0.4f } } },

        { "Pad", "Soft Pad",
          { { voiceMode, poly }, { osc1Wave, W::triSaw }, { osc2Wave, W::tri }, { osc2Tune, 0.12f },
            { osc3Wave, W::saw }, { osc3Range, R::ft16 }, { mixOsc2, 0.8f }, { mixOsc3, 0.35f },
            { cutoff, 900.0f }, { emphasis, 0.2f }, { contour, 0.3f },
            { fAttack, 1.5f }, { fDecay, 2.0f }, { fSustain, 0.5f }, { fRelease, 1.5f },
            { aAttack, 1.2f }, { aDecay, 1.0f }, { aSustain, 1.0f }, { aRelease, 1.6f },
            { velAmp, 0.2f } } },

        { "FX", "Laser Zap",
          { { voiceMode, mono }, { osc1Wave, W::square }, { cutoff, 90.0f }, { emphasis, 0.88f },
            { contour, 1.0f }, { fAttack, 0.001f }, { fDecay, 0.18f }, { fSustain, 0.0f },
            { aDecay, 0.4f }, { aSustain, 0.0f }, { aRelease, 0.3f }, { legato, 0.0f } } },

        { "Bass", "Filter Wobble",
          { { voiceMode, mono }, { osc1Wave, W::saw }, { osc1Range, R::ft16 }, { osc2Wave, W::saw },
            { osc2Range, R::ft16 }, { osc2Tune, 0.05f }, { mixOsc2, 0.8f },
            { osc3Range, R::lo }, { osc3Wave, W::tri }, { osc3Tune, -3.0f }, { osc3KeyTrack, 0.0f },
            { cutoff, 400.0f }, { emphasis, 0.55f }, { contour, 0.2f }, { drive, 0.3f },
            { filterMod, 1.0f }, { modAmount, 0.7f }, { aSustain, 1.0f } } },

        // ---- added with the arpeggiator, trance gate and effects.
        // Appended rather than sorted in, so saved program numbers keep pointing at the same sound.

        { "Arp", "Trance Arp",
          { { voiceMode, poly }, { osc1Wave, W::saw }, { osc2Wave, W::saw }, { osc2Tune, 0.12f },
            { osc3Wave, W::saw }, { osc3Tune, -0.1f }, { mixOsc2, 0.85f }, { mixOsc3, 0.6f },
            { cutoff, 1400.0f }, { emphasis, 0.3f }, { contour, 0.45f }, { keyTrack, 2.0f },
            { fAttack, 0.001f }, { fDecay, 0.22f }, { fSustain, 0.2f }, { fRelease, 0.2f },
            { aAttack, 0.001f }, { aDecay, 0.3f }, { aSustain, 0.6f }, { aRelease, 0.15f },
            { arpOn, 1.0f }, { arpMode, Arp::up }, { arpRate, ArpRate::s }, { arpOctaves, 2.0f }, { arpGate, 0.45f },
            { chorusOn, 1.0f }, { chorusMix, 0.35f },
            { delayOn, 1.0f }, { delayTime, DelayTime::e_dot }, { delayFeedback, 0.4f }, { delayMix, 0.3f },
            { reverbOn, 1.0f }, { reverbSize, 0.7f }, { reverbMix, 0.25f } } },

        { "Arp", "Bouncing Octaves",
          { { voiceMode, mono }, { legato, 0.0f }, { osc1Wave, W::square }, { osc2Wave, W::narrow }, { osc2Range, R::ft4 },
            { mixOsc2, 0.35f }, { cutoff, 900.0f }, { emphasis, 0.45f }, { contour, 0.5f },
            { fAttack, 0.001f }, { fDecay, 0.15f }, { fSustain, 0.1f },
            { aAttack, 0.001f }, { aDecay, 0.2f }, { aSustain, 0.5f }, { aRelease, 0.08f },
            { arpOn, 1.0f }, { arpMode, Arp::upDown }, { arpRate, ArpRate::s }, { arpOctaves, 3.0f }, { arpGate, 0.3f },
            { delayOn, 1.0f }, { delayTime, DelayTime::e }, { delayFeedback, 0.3f }, { delayMix, 0.2f } } },

        { "Arp", "Random Bells",
          { { voiceMode, poly }, { osc1Wave, W::tri }, { osc2Wave, W::tri }, { osc2Range, R::ft4 }, { osc2Tune, 7.0f },
            { mixOsc2, 0.5f }, { cutoff, 2600.0f }, { emphasis, 0.1f }, { contour, 0.3f },
            { fAttack, 0.001f }, { fDecay, 0.6f }, { fSustain, 0.0f },
            { aAttack, 0.001f }, { aDecay, 1.2f }, { aSustain, 0.0f }, { aRelease, 0.8f },
            { arpOn, 1.0f }, { arpMode, Arp::random }, { arpRate, ArpRate::e }, { arpOctaves, 2.0f }, { arpGate, 0.6f },
            { delayOn, 1.0f }, { delayTime, DelayTime::q_dot }, { delayFeedback, 0.45f }, { delayDamp, 0.5f }, { delayMix, 0.3f },
            { reverbOn, 1.0f }, { reverbSize, 0.85f }, { reverbMix, 0.35f } } },

        { "Arp", "Swing Pluck Arp",
          { { voiceMode, poly }, { osc1Wave, W::wide }, { osc2Wave, W::saw }, { osc2Tune, 0.06f }, { mixOsc2, 0.5f },
            { cutoff, 500.0f }, { emphasis, 0.4f }, { contour, 0.75f },
            { fAttack, 0.001f }, { fDecay, 0.2f }, { fSustain, 0.0f }, { fRelease, 0.2f },
            { aAttack, 0.001f }, { aDecay, 0.5f }, { aSustain, 0.0f }, { aRelease, 0.3f }, { velFilter, 0.3f },
            { arpOn, 1.0f }, { arpMode, Arp::asPlayed }, { arpRate, ArpRate::s }, { arpSwing, 0.35f }, { arpGate, 0.5f },
            { reverbOn, 1.0f }, { reverbSize, 0.5f }, { reverbMix, 0.2f } } },

        { "Arp", "Chord Stabs",
          { { voiceMode, poly }, { osc1Wave, W::saw }, { osc2Wave, W::saw }, { osc2Tune, 0.1f }, { mixOsc2, 0.9f },
            { cutoff, 700.0f }, { emphasis, 0.25f }, { contour, 0.6f },
            { fAttack, 0.001f }, { fDecay, 0.18f }, { fSustain, 0.05f },
            { aAttack, 0.001f }, { aDecay, 0.25f }, { aSustain, 0.4f }, { aRelease, 0.1f },
            { arpOn, 1.0f }, { arpMode, Arp::chord }, { arpRate, ArpRate::e }, { arpGate, 0.25f },
            { chorusOn, 1.0f }, { chorusMix, 0.4f },
            { delayOn, 1.0f }, { delayTime, DelayTime::e_dot }, { delayFeedback, 0.35f }, { delayMix, 0.25f },
            { reverbOn, 1.0f }, { reverbMix, 0.2f } } },

        { "Bass", "Rolling Bass",
          { { voiceMode, mono }, { legato, 0.0f }, { osc1Range, R::ft16 }, { osc1Wave, W::saw },
            { osc2Range, R::ft16 }, { osc2Wave, W::saw }, { osc2Tune, -0.06f }, { mixOsc2, 0.8f },
            { cutoff, 320.0f }, { emphasis, 0.3f }, { contour, 0.55f }, { drive, 0.3f },
            { fAttack, 0.001f }, { fDecay, 0.12f }, { fSustain, 0.05f },
            { aAttack, 0.001f }, { aDecay, 0.2f }, { aSustain, 0.7f }, { aRelease, 0.05f },
            { arpOn, 1.0f }, { arpRate, ArpRate::s }, { arpGate, 0.6f } } },

        { "Gated", "Gated Supersaw",
          gatePattern ({ { voiceMode, poly }, { osc1Wave, W::saw }, { osc2Wave, W::saw }, { osc2Tune, 0.12f },
                         { osc3Wave, W::saw }, { osc3Tune, -0.12f }, { mixOsc2, 0.9f }, { mixOsc3, 0.8f },
                         { cutoff, 3200.0f }, { emphasis, 0.15f }, { contour, 0.2f },
                         { aAttack, 0.02f }, { aSustain, 1.0f }, { aRelease, 0.6f },
                         { gateOn, 1.0f }, { gateRate, GateRate::s }, { gateDepth, 1.0f }, { gateLength, 0.7f }, { gateSmooth, 0.15f },
                         { chorusOn, 1.0f }, { chorusMix, 0.6f },
                         { delayOn, 1.0f }, { delayTime, DelayTime::e_dot }, { delayFeedback, 0.35f }, { delayMix, 0.25f },
                         { reverbOn, 1.0f }, { reverbSize, 0.75f }, { reverbMix, 0.3f } },
                       "x.xx.xx.x.xx.xx.") },

        { "Gated", "Stutter Bass",
          gatePattern ({ { voiceMode, mono }, { osc1Range, R::ft16 }, { osc1Wave, W::saw }, { osc2Range, R::ft32 },
                         { osc2Wave, W::square }, { mixOsc2, 0.6f }, { cutoff, 450.0f }, { emphasis, 0.35f },
                         { contour, 0.3f }, { drive, 0.4f }, { aSustain, 1.0f }, { aRelease, 0.1f },
                         { gateOn, 1.0f }, { gateRate, GateRate::s }, { gateDepth, 0.95f }, { gateLength, 0.6f }, { gateSmooth, 0.1f } },
                       "xxx.xx.xxxx.x.x.") },

        { "Gated", "Pulse Strings",
          gatePattern ({ { voiceMode, poly }, { osc1Wave, W::triSaw }, { osc2Wave, W::saw }, { osc2Tune, 0.08f },
                         { mixOsc2, 0.7f }, { cutoff, 1800.0f }, { emphasis, 0.1f }, { contour, 0.2f },
                         { fAttack, 0.4f }, { fSustain, 0.6f }, { aAttack, 0.3f }, { aSustain, 1.0f }, { aRelease, 1.0f },
                         { gateOn, 1.0f }, { gateRate, GateRate::s_tri }, { gateDepth, 0.8f }, { gateLength, 0.55f }, { gateSmooth, 0.3f },
                         { chorusOn, 1.0f }, { reverbOn, 1.0f }, { reverbSize, 0.8f }, { reverbMix, 0.35f } },
                       "xx.xx.xx.xx.xx.x") },

        { "Pad", "Wide Strings",
          { { voiceMode, poly }, { osc1Wave, W::saw }, { osc2Wave, W::saw }, { osc2Tune, 0.1f },
            { osc3Wave, W::saw }, { osc3Range, R::ft16 }, { osc3Tune, -0.08f }, { mixOsc2, 0.9f }, { mixOsc3, 0.5f },
            { cutoff, 2200.0f }, { emphasis, 0.1f }, { contour, 0.2f },
            { fAttack, 0.8f }, { fDecay, 1.5f }, { fSustain, 0.7f }, { fRelease, 1.2f },
            { aAttack, 0.6f }, { aDecay, 1.0f }, { aSustain, 1.0f }, { aRelease, 1.5f }, { velAmp, 0.2f },
            { chorusOn, 1.0f }, { chorusRate, 0.4f }, { chorusDepth, 0.7f }, { chorusMix, 0.7f },
            { reverbOn, 1.0f }, { reverbSize, 0.85f }, { reverbMix, 0.4f } } },

        { "Pad", "Dark Space",
          { { voiceMode, poly }, { osc1Wave, W::tri }, { osc2Wave, W::triSaw }, { osc2Range, R::ft16 }, { osc2Tune, 0.15f },
            { mixOsc2, 0.6f }, { mixNoise, 0.15f }, { noiseColour, 1.0f },
            { cutoff, 600.0f }, { emphasis, 0.35f }, { contour, 0.35f },
            { fAttack, 3.0f }, { fDecay, 3.0f }, { fSustain, 0.4f }, { fRelease, 3.0f },
            { aAttack, 2.0f }, { aSustain, 1.0f }, { aRelease, 3.0f },
            { delayOn, 1.0f }, { delayTime, DelayTime::h }, { delayFeedback, 0.5f }, { delayDamp, 0.7f }, { delayMix, 0.2f },
            { reverbOn, 1.0f }, { reverbSize, 0.95f }, { reverbDamp, 0.6f }, { reverbMix, 0.5f } } },

        { "Lead", "Echo Lead",
          { { voiceMode, mono }, { osc1Wave, W::square }, { osc2Wave, W::saw }, { osc2Tune, 0.08f }, { mixOsc2, 0.7f },
            { osc3Range, R::lo }, { osc3Wave, W::tri }, { osc3KeyTrack, 0.0f }, { oscMod, 1.0f },
            { cutoff, 1600.0f }, { emphasis, 0.35f }, { contour, 0.35f }, { keyTrack, 2.0f },
            { fAttack, 0.005f }, { fDecay, 0.4f }, { fSustain, 0.6f },
            { aSustain, 1.0f }, { aRelease, 0.2f }, { glide, 0.06f },
            { delayOn, 1.0f }, { delayTime, DelayTime::q_dot }, { delayFeedback, 0.45f }, { delayMix, 0.35f },
            { reverbOn, 1.0f }, { reverbSize, 0.7f }, { reverbMix, 0.25f } } },

        { "Lead", "Screamer",
          { { voiceMode, mono }, { osc1Wave, W::saw }, { osc2Wave, W::saw }, { osc2Tune, 0.15f }, { osc3Wave, W::saw },
            { osc3Range, R::ft16 }, { mixOsc2, 1.0f }, { mixOsc3, 0.7f },
            { cutoff, 2400.0f }, { emphasis, 0.5f }, { contour, 0.3f }, { drive, 0.8f },
            { fAttack, 0.01f }, { fDecay, 0.3f }, { fSustain, 0.7f },
            { aSustain, 1.0f }, { aRelease, 0.25f }, { glide, 0.03f },
            { chorusOn, 1.0f }, { chorusMix, 0.3f },
            { delayOn, 1.0f }, { delayTime, DelayTime::e_dot }, { delayFeedback, 0.3f }, { delayMix, 0.22f },
            { reverbOn, 1.0f }, { reverbMix, 0.2f } } },

        { "Keys", "Poly Keys",
          { { voiceMode, poly }, { osc1Wave, W::square }, { osc2Wave, W::tri }, { osc2Range, R::ft4 }, { mixOsc2, 0.5f },
            { cutoff, 1500.0f }, { emphasis, 0.15f }, { contour, 0.4f }, { keyTrack, 2.0f },
            { fAttack, 0.001f }, { fDecay, 0.8f }, { fSustain, 0.3f }, { fRelease, 0.4f },
            { aAttack, 0.002f }, { aDecay, 1.5f }, { aSustain, 0.4f }, { aRelease, 0.4f }, { velFilter, 0.35f },
            { chorusOn, 1.0f }, { chorusRate, 0.8f }, { chorusMix, 0.45f },
            { reverbOn, 1.0f }, { reverbSize, 0.45f }, { reverbMix, 0.18f } } },

        { "FX", "Noise Riser",
          { { voiceMode, poly }, { mixOsc1, 0.0f }, { mixNoise, 1.0f },
            { cutoff, 120.0f }, { emphasis, 0.65f }, { contour, 1.0f }, { keyTrack, 3.0f },
            { fAttack, 6.0f }, { fDecay, 2.0f }, { fSustain, 1.0f }, { fRelease, 2.0f },
            { aAttack, 3.0f }, { aSustain, 1.0f }, { aRelease, 2.5f },
            { delayOn, 1.0f }, { delayTime, DelayTime::q }, { delayFeedback, 0.55f }, { delayMix, 0.3f },
            { reverbOn, 1.0f }, { reverbSize, 0.9f }, { reverbMix, 0.45f } } },

        { "FX", "Computer Chatter",
          { { voiceMode, mono }, { legato, 0.0f }, { osc1Wave, W::square }, { osc1Range, R::ft4 },
            { cutoff, 1800.0f }, { emphasis, 0.7f }, { contour, 0.5f },
            { fAttack, 0.001f }, { fDecay, 0.06f }, { fSustain, 0.0f },
            { aAttack, 0.001f }, { aDecay, 0.08f }, { aSustain, 0.0f }, { aRelease, 0.05f },
            { arpOn, 1.0f }, { arpMode, Arp::random }, { arpRate, ArpRate::t }, { arpOctaves, 4.0f }, { arpGate, 0.5f },
            { delayOn, 1.0f }, { delayTime, DelayTime::s }, { delayFeedback, 0.4f }, { delayMix, 0.25f } } },
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
