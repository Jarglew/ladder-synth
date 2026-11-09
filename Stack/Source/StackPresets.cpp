#include "StackPresets.h"
#include "StackParameters.h"

namespace stack
{

namespace
{
using Values = std::vector<std::pair<juce::String, float>>;

// Choice indices, to keep the tables readable.
namespace Wave { constexpr float saw = 0, square = 1, tri = 2, sine = 3, supersaw = 4, noise = 5; }
namespace Filt { constexpr float off = 0, lp24 = 1, lp12 = 2, hp12 = 3, bp12 = 4; }
namespace Rate { constexpr float q = 0, e = 1, eT = 2, s = 3, sT = 4, t = 5; }
namespace Arp { constexpr float up = 0, down = 1, upDown = 2, downUp = 3, random = 4, played = 5, chord = 6; }
namespace Delay { constexpr float half = 0, qDot = 1, q = 2, eDot = 3, e = 4, eT = 5, s = 6; }

// Small builder so each preset reads as a list of what makes it different.
struct P
{
    Values v;

    P& layer (int l, std::initializer_list<std::pair<const char*, float>> settings)
    {
        v.push_back ({ layerId (l, LayerParam::on), 1.0f });
        for (const auto& [suffix, value] : settings)
            v.push_back ({ layerId (l, suffix), value });
        return *this;
    }

    P& set (std::initializer_list<std::pair<const char*, float>> settings)
    {
        for (const auto& [id, value] : settings)
            v.push_back ({ id, value });
        return *this;
    }
};

using namespace LayerParam;
using namespace GlobalParam;

FactoryPreset make (const char* name, const char* category, P p, const char* arpPattern = "", const char* gatePattern = "")
{
    return { name, category, std::move (p.v), arpPattern, gatePattern };
}
} // namespace

juce::StringArray presetCategories()
{
    return { "Lead", "Pad", "Gated", "Bass", "Pluck", "Keys", "Arp", "FX" };
}

const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets {
        make ("Init", "Lead", P {}),

        // ------------------------------------------------------------ leads
        make ("Supersaw Anthem", "Lead",
              P {}.layer (0, { { wave, Wave::supersaw }, { detune, 0.45f }, { width, 0.85f }, { cutoff, 9000 }, { aAttack, 0.01f }, { aSustain, 1 }, { aRelease, 0.35f } })
                  .layer (1, { { wave, Wave::supersaw }, { octave, 1 }, { level, 0.45f }, { detune, 0.35f }, { cutoff, 9000 }, { aSustain, 1 }, { aRelease, 0.35f } })
                  .layer (2, { { wave, Wave::saw }, { octave, -1 }, { level, 0.4f }, { cutoff, 3000 }, { aSustain, 1 }, { aRelease, 0.3f } })
                  .set ({ { delayOn, 1 }, { delayTime, Delay::eDot }, { delayMix, 0.2f }, { reverbOn, 1 }, { reverbSize, 0.7f }, { reverbMix, 0.25f }, { volume, -10 } })),

        make ("Hard Saw Lead", "Lead",
              P {}.layer (0, { { wave, Wave::saw }, { cutoff, 4500 }, { resonance, 0.25f }, { filterEnv, 0.3f }, { fDecay, 0.3f }, { fSustain, 0.5f }, { aSustain, 1 }, { aRelease, 0.15f } })
                  .layer (1, { { wave, Wave::square }, { semitone, 7 }, { level, 0.35f }, { cutoff, 4000 }, { aSustain, 1 }, { aRelease, 0.15f } })
                  .set ({ { voiceMode, 1 }, { glide, 0.06f }, { distOn, 1 }, { distDrive, 0.35f }, { delayOn, 1 }, { delayTime, Delay::e }, { delayMix, 0.15f } })),

        make ("Soft Sine Lead", "Lead",
              P {}.layer (0, { { wave, Wave::sine }, { filterType, Filt::off }, { aAttack, 0.02f }, { aSustain, 1 }, { aRelease, 0.4f } })
                  .layer (1, { { wave, Wave::tri }, { octave, 1 }, { level, 0.3f }, { filterType, Filt::off }, { aSustain, 1 }, { aRelease, 0.4f } })
                  .set ({ { voiceMode, 1 }, { glide, 0.1f }, { lfo2Pitch, 0.15f }, { delayOn, 1 }, { delayMix, 0.25f }, { reverbOn, 1 }, { reverbMix, 0.3f } })),

        // ------------------------------------------------------------ pads
        make ("Wide Saw Pad", "Pad",
              P {}.layer (0, { { wave, Wave::supersaw }, { detune, 0.3f }, { width, 1 }, { cutoff, 2500 }, { filterEnv, 0.25f }, { fAttack, 1.2f }, { fDecay, 2 }, { fSustain, 0.6f }, { aAttack, 0.9f }, { aSustain, 1 }, { aRelease, 1.8f } })
                  .layer (1, { { wave, Wave::saw }, { octave, -1 }, { level, 0.4f }, { cutoff, 1200 }, { aAttack, 0.9f }, { aSustain, 1 }, { aRelease, 1.8f } })
                  .set ({ { chorusOn, 1 }, { reverbOn, 1 }, { reverbSize, 0.85f }, { reverbMix, 0.4f } })),

        make ("Glass Pad", "Pad",
              P {}.layer (0, { { wave, Wave::sine }, { filterType, Filt::off }, { aAttack, 1.5f }, { aSustain, 1 }, { aRelease, 2.5f } })
                  .layer (1, { { wave, Wave::tri }, { octave, 1 }, { level, 0.4f }, { filterType, Filt::off }, { aAttack, 1.5f }, { aSustain, 1 }, { aRelease, 2.5f } })
                  .layer (2, { { wave, Wave::supersaw }, { octave, 1 }, { level, 0.2f }, { filterType, Filt::hp12 }, { cutoff, 2000 }, { aAttack, 2 }, { aSustain, 1 }, { aRelease, 2.5f } })
                  .set ({ { phaserOn, 1 }, { phaserMix, 0.4f }, { reverbOn, 1 }, { reverbSize, 0.9f }, { reverbMix, 0.45f }, { volume, -9 } })),

        make ("Dark Drone", "Pad",
              P {}.layer (0, { { wave, Wave::saw }, { octave, -1 }, { cutoff, 400 }, { resonance, 0.4f }, { aAttack, 2 }, { aSustain, 1 }, { aRelease, 3 } })
                  .layer (1, { { wave, Wave::square }, { octave, -2 }, { level, 0.5f }, { cutoff, 300 }, { aAttack, 2 }, { aSustain, 1 }, { aRelease, 3 } })
                  .set ({ { lfo1Cutoff, 0.35f }, { lfo1Rate, 0.15f }, { reverbOn, 1 }, { reverbMix, 0.45f }, { reverbSize, 0.9f } })),

        // ------------------------------------------------------------ gated
        make ("Trance Gate Pad", "Gated",
              P {}.layer (0, { { wave, Wave::supersaw }, { detune, 0.35f }, { width, 1 }, { cutoff, 4000 }, { aAttack, 0.05f }, { aSustain, 1 }, { aRelease, 0.6f } })
                  .layer (1, { { wave, Wave::saw }, { octave, -1 }, { level, 0.45f }, { cutoff, 2000 }, { aAttack, 0.05f }, { aSustain, 1 }, { aRelease, 0.6f } })
                  .set ({ { gateOn, 1 }, { gateRate, Rate::s }, { delayOn, 1 }, { delayTime, Delay::eDot }, { delayMix, 0.2f }, { reverbOn, 1 }, { reverbMix, 0.3f }, { volume, -9 } }),
              "", "x.x.xx.xx.xxx.x."),

        make ("Stutter Chords", "Gated",
              P {}.layer (0, { { wave, Wave::supersaw }, { cutoff, 6000 }, { aSustain, 1 }, { aRelease, 0.3f } })
                  .layer (1, { { wave, Wave::square }, { octave, -1 }, { level, 0.45f }, { cutoff, 2500 }, { aSustain, 1 }, { aRelease, 0.3f } })
                  .set ({ { gateOn, 1 }, { gateRate, Rate::s }, { gateDepth, 0.9f }, { reverbOn, 1 }, { reverbMix, 0.2f }, { volume, -10 } }),
              "", "xx.xx.x.xx.x.x.x"),

        make ("Half-Time Swell", "Gated",
              P {}.layer (0, { { wave, Wave::saw }, { cutoff, 1800 }, { filterEnv, 0.3f }, { fAttack, 0.5f }, { fSustain, 0.7f }, { aAttack, 0.3f }, { aSustain, 1 }, { aRelease, 1 } })
                  .layer (1, { { wave, Wave::tri }, { octave, 1 }, { level, 0.4f }, { filterType, Filt::off }, { aAttack, 0.3f }, { aSustain, 1 }, { aRelease, 1 } })
                  .set ({ { gateOn, 1 }, { gateRate, Rate::e }, { gateAttack, 20 }, { gateRelease, 120 }, { chorusOn, 1 }, { reverbOn, 1 }, { reverbMix, 0.35f } }),
              "", "x.xxx.x.x.xxx.xx"),

        // ------------------------------------------------------------ bass
        make ("Saw Bass", "Bass",
              P {}.layer (0, { { wave, Wave::saw }, { octave, -1 }, { cutoff, 600 }, { resonance, 0.2f }, { filterEnv, 0.45f }, { fDecay, 0.25f }, { fSustain, 0.2f }, { aAttack, 0.002f }, { aSustain, 1 }, { aRelease, 0.1f } })
                  .layer (1, { { wave, Wave::square }, { octave, -2 }, { level, 0.6f }, { cutoff, 300 }, { aAttack, 0.002f }, { aSustain, 1 }, { aRelease, 0.1f } })
                  .set ({ { voiceMode, 1 }, { distOn, 1 }, { distDrive, 0.2f } })),

        make ("Sub Bass", "Bass",
              P {}.layer (0, { { wave, Wave::sine }, { octave, -1 }, { filterType, Filt::off }, { aAttack, 0.003f }, { aSustain, 1 }, { aRelease, 0.08f }, { velocity, 0.2f } })
                  .layer (1, { { wave, Wave::tri }, { octave, -1 }, { level, 0.3f }, { filterType, Filt::off }, { aSustain, 1 }, { aRelease, 0.08f } })
                  .set ({ { voiceMode, 1 } })),

        make ("Reese Bass", "Bass",
              P {}.layer (0, { { wave, Wave::saw }, { octave, -1 }, { fine, -12 }, { cutoff, 900 }, { resonance, 0.15f }, { aSustain, 1 }, { aRelease, 0.15f } })
                  .layer (1, { { wave, Wave::saw }, { octave, -1 }, { fine, 12 }, { cutoff, 900 }, { resonance, 0.15f }, { aSustain, 1 }, { aRelease, 0.15f } })
                  .layer (2, { { wave, Wave::sine }, { octave, -2 }, { level, 0.6f }, { filterType, Filt::off }, { aSustain, 1 }, { aRelease, 0.15f } })
                  .set ({ { voiceMode, 1 }, { lfo1Cutoff, 0.15f }, { lfo1Rate, 0.3f }, { distOn, 1 }, { distDrive, 0.15f } })),

        make ("Pluck Bass", "Bass",
              P {}.layer (0, { { wave, Wave::square }, { octave, -1 }, { cutoff, 300 }, { resonance, 0.35f }, { filterEnv, 0.6f }, { fDecay, 0.18f }, { fSustain, 0 }, { aDecay, 0.4f }, { aSustain, 0.3f }, { aRelease, 0.1f } })
                  .set ({ { voiceMode, 1 } })),

        // ------------------------------------------------------------ plucks
        make ("Trance Pluck", "Pluck",
              P {}.layer (0, { { wave, Wave::supersaw }, { cutoff, 600 }, { resonance, 0.2f }, { filterEnv, 0.65f }, { fDecay, 0.22f }, { fSustain, 0 }, { aDecay, 0.45f }, { aSustain, 0 }, { aRelease, 0.3f } })
                  .layer (1, { { wave, Wave::sine }, { octave, 1 }, { level, 0.25f }, { filterType, Filt::off }, { aDecay, 0.2f }, { aSustain, 0 }, { aRelease, 0.2f } })
                  .set ({ { delayOn, 1 }, { delayTime, Delay::eDot }, { delayMix, 0.3f }, { delayFeedback, 0.45f }, { reverbOn, 1 }, { reverbMix, 0.3f } })),

        make ("Bell Pluck", "Pluck",
              P {}.layer (0, { { wave, Wave::sine }, { filterType, Filt::off }, { aDecay, 1.2f }, { aSustain, 0 }, { aRelease, 0.8f } })
                  .layer (1, { { wave, Wave::sine }, { octave, 1 }, { semitone, 7 }, { level, 0.35f }, { filterType, Filt::off }, { aDecay, 0.6f }, { aSustain, 0 }, { aRelease, 0.5f } })
                  .layer (2, { { wave, Wave::sine }, { octave, 2 }, { level, 0.15f }, { filterType, Filt::off }, { aDecay, 0.3f }, { aSustain, 0 }, { aRelease, 0.3f } })
                  .set ({ { reverbOn, 1 }, { reverbMix, 0.35f }, { volume, -10 } })),

        // ------------------------------------------------------------ keys
        make ("Soft Keys", "Keys",
              P {}.layer (0, { { wave, Wave::tri }, { filterType, Filt::lp12 }, { cutoff, 3000 }, { aDecay, 1.5f }, { aSustain, 0.3f }, { aRelease, 0.4f }, { velocity, 0.7f } })
                  .layer (1, { { wave, Wave::sine }, { octave, 1 }, { level, 0.35f }, { filterType, Filt::off }, { aDecay, 0.8f }, { aSustain, 0.1f }, { aRelease, 0.3f }, { velocity, 0.7f } })
                  .set ({ { chorusOn, 1 }, { chorusMix, 0.35f }, { reverbOn, 1 }, { reverbMix, 0.2f }, { volume, -10 } })),

        make ("Drawbar Organ", "Keys",
              P {}.layer (0, { { wave, Wave::sine }, { filterType, Filt::off }, { aAttack, 0.005f }, { aSustain, 1 }, { aRelease, 0.05f }, { velocity, 0 } })
                  .layer (1, { { wave, Wave::sine }, { octave, 1 }, { level, 0.7f }, { filterType, Filt::off }, { aSustain, 1 }, { aRelease, 0.05f }, { velocity, 0 } })
                  .layer (2, { { wave, Wave::sine }, { octave, 2 }, { level, 0.45f }, { filterType, Filt::off }, { aSustain, 1 }, { aRelease, 0.05f }, { velocity, 0 } })
                  .layer (3, { { wave, Wave::sine }, { octave, -1 }, { level, 0.5f }, { filterType, Filt::off }, { aSustain, 1 }, { aRelease, 0.05f }, { velocity, 0 } })
                  .set ({ { chorusOn, 1 }, { chorusRate, 0.6f }, { chorusDepth, 0.5f }, { chorusMix, 0.5f }, { volume, -14 } })),

        make ("Brass Stab", "Keys",
              P {}.layer (0, { { wave, Wave::saw }, { cutoff, 500 }, { filterEnv, 0.55f }, { fAttack, 0.04f }, { fDecay, 0.35f }, { fSustain, 0.45f }, { aAttack, 0.02f }, { aSustain, 0.9f }, { aRelease, 0.2f } })
                  .layer (1, { { wave, Wave::saw }, { fine, 8 }, { cutoff, 500 }, { filterEnv, 0.55f }, { fAttack, 0.04f }, { fDecay, 0.35f }, { fSustain, 0.45f }, { aAttack, 0.02f }, { aSustain, 0.9f }, { aRelease, 0.2f } })
                  .set ({ { reverbOn, 1 }, { reverbMix, 0.2f }, { volume, -12 } })),

        // ------------------------------------------------------------ arps
        make ("Classic Arp", "Arp",
              P {}.layer (0, { { wave, Wave::saw }, { cutoff, 1200 }, { resonance, 0.3f }, { filterEnv, 0.4f }, { fDecay, 0.15f }, { fSustain, 0 }, { aDecay, 0.25f }, { aSustain, 0 }, { aRelease, 0.15f } })
                  .set ({ { arpOn, 1 }, { arpMode, Arp::up }, { arpRate, Rate::s }, { arpOctaves, 2 }, { arpGate, 0.5f },
                          { delayOn, 1 }, { delayTime, Delay::eDot }, { delayMix, 0.25f }, { reverbOn, 1 }, { reverbMix, 0.2f } })),

        make ("Up Down Sparkle", "Arp",
              P {}.layer (0, { { wave, Wave::sine }, { octave, 1 }, { filterType, Filt::off }, { aDecay, 0.3f }, { aSustain, 0 }, { aRelease, 0.2f } })
                  .layer (1, { { wave, Wave::tri }, { level, 0.5f }, { filterType, Filt::off }, { aDecay, 0.3f }, { aSustain, 0 }, { aRelease, 0.2f } })
                  .set ({ { arpOn, 1 }, { arpMode, Arp::upDown }, { arpRate, Rate::s }, { arpOctaves, 3 }, { arpGate, 0.35f },
                          { delayOn, 1 }, { delayMix, 0.3f }, { reverbOn, 1 }, { reverbMix, 0.35f } })),

        make ("Accent Sequence", "Arp",
              P {}.layer (0, { { wave, Wave::square }, { octave, -1 }, { cutoff, 700 }, { resonance, 0.5f }, { filterEnv, 0.5f }, { fDecay, 0.12f }, { fSustain, 0 }, { aDecay, 0.3f }, { aSustain, 0.2f }, { aRelease, 0.1f }, { velocity, 0.8f } })
                  .set ({ { arpOn, 1 }, { arpMode, Arp::played }, { arpRate, Rate::s }, { arpGate, 0.55f },
                          { distOn, 1 }, { distDrive, 0.25f }, { delayOn, 1 }, { delayTime, Delay::eDot }, { delayMix, 0.2f } }),
              "Xx.xXxx.Xx.xXx.x"),

        make ("Chord Pulse", "Arp",
              P {}.layer (0, { { wave, Wave::supersaw }, { cutoff, 5000 }, { aDecay, 0.3f }, { aSustain, 0.5f }, { aRelease, 0.2f } })
                  .set ({ { arpOn, 1 }, { arpMode, Arp::chord }, { arpRate, Rate::e }, { arpGate, 0.4f }, { reverbOn, 1 }, { reverbMix, 0.3f } })),

        // ------------------------------------------------------------ fx
        make ("Noise Riser", "FX",
              P {}.layer (0, { { wave, Wave::noise }, { filterType, Filt::bp12 }, { cutoff, 300 }, { resonance, 0.6f }, { filterEnv, 1 }, { fAttack, 6 }, { fDecay, 1 }, { fSustain, 1 }, { aAttack, 4 }, { aSustain, 1 }, { aRelease, 2 }, { keyTrack, 0 } })
                  .layer (1, { { wave, Wave::supersaw }, { filterType, Filt::hp12 }, { cutoff, 3000 }, { level, 0.3f }, { aAttack, 4 }, { aSustain, 1 }, { aRelease, 2 } })
                  .set ({ { phaserOn, 1 }, { reverbOn, 1 }, { reverbMix, 0.5f }, { reverbSize, 0.9f } })),

        make ("Laser Drop", "FX",
              P {}.layer (0, { { wave, Wave::square }, { cutoff, 150 }, { resonance, 0.85f }, { filterEnv, 1 }, { fDecay, 0.25f }, { fSustain, 0 }, { aDecay, 0.5f }, { aSustain, 0 }, { aRelease, 0.3f } })
                  .set ({ { voiceMode, 1 }, { glide, 0.3f }, { delayOn, 1 }, { delayMix, 0.3f } })),

        make ("Wobble", "FX",
              P {}.layer (0, { { wave, Wave::saw }, { octave, -1 }, { cutoff, 300 }, { resonance, 0.45f }, { aSustain, 1 }, { aRelease, 0.1f } })
                  .layer (1, { { wave, Wave::square }, { octave, -2 }, { level, 0.5f }, { cutoff, 250 }, { aSustain, 1 }, { aRelease, 0.1f } })
                  .set ({ { voiceMode, 1 }, { lfo1Cutoff, 0.8f }, { lfo1Rate, 3 }, { distOn, 1 }, { distDrive, 0.3f } })),
    };

    return presets;
}

juce::File userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Layer Stack").getChildFile ("Presets");
}

std::vector<UserPresetInfo> scanUserPresets()
{
    std::vector<UserPresetInfo> result;
    const auto folder = userPresetFolder();
    if (! folder.isDirectory())
        return result;

    for (const auto& file : folder.findChildFiles (juce::File::findFiles, true, "*.lspreset"))
    {
        if (auto xml = juce::XmlDocument::parse (file))
            result.push_back ({ file, xml->getStringAttribute ("name", file.getFileNameWithoutExtension()),
                                xml->getStringAttribute ("category", "User") });
    }

    std::sort (result.begin(), result.end(), [] (const UserPresetInfo& a, const UserPresetInfo& b)
    {
        return a.name.compareNatural (b.name) < 0;
    });
    return result;
}

} // namespace stack
