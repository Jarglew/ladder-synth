#include "StackParameters.h"

namespace stack
{

namespace
{
using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;
using API = juce::AudioParameterInt;
using Range = juce::NormalisableRange<float>;

juce::ParameterID pid (const juce::String& id) { return { id, 1 }; }

std::unique_ptr<APF> unit (const juce::String& id, const juce::String& name, float def)
{
    return std::make_unique<APF> (pid (id), name, Range (0.0f, 1.0f), def);
}

std::unique_ptr<APF> seconds (const juce::String& id, const juce::String& name, float def)
{
    return std::make_unique<APF> (pid (id), name, Range (0.001f, 10.0f, 0.0f, 0.25f), def,
                                  juce::AudioParameterFloatAttributes().withLabel ("s"));
}

std::unique_ptr<APF> hertz (const juce::String& id, const juce::String& name, float lo, float hi, float def)
{
    auto r = Range (lo, hi);
    r.setSkewForCentre (std::sqrt (lo * hi));
    return std::make_unique<APF> (pid (id), name, r, def, juce::AudioParameterFloatAttributes().withLabel ("Hz"));
}

// Order matters: StackParameterReader indexes layer parameters in this order.
const char* layerSuffixes[] = {
    LayerParam::on, LayerParam::wave, LayerParam::octave, LayerParam::semitone, LayerParam::fine,
    LayerParam::detune, LayerParam::width, LayerParam::level, LayerParam::pan, LayerParam::filterType,
    LayerParam::cutoff, LayerParam::resonance, LayerParam::filterEnv, LayerParam::keyTrack,
    LayerParam::fAttack, LayerParam::fDecay, LayerParam::fSustain, LayerParam::fRelease,
    LayerParam::aAttack, LayerParam::aDecay, LayerParam::aSustain, LayerParam::aRelease,
    LayerParam::velocity, LayerParam::root, LayerParam::loop
};
static_assert (sizeof (layerSuffixes) / sizeof (layerSuffixes[0]) == 25);
} // namespace

const juce::StringArray& waveNames()
{
    static const juce::StringArray names { "Saw", "Square", "Triangle", "Sine", "Supersaw", "Noise", "Sample" };
    return names;
}

const juce::StringArray& filterTypeNames()
{
    static const juce::StringArray names { "Off", "LP 24", "LP 12", "HP 12", "BP 12" };
    return names;
}

const juce::StringArray& rateNames()
{
    static const juce::StringArray names { "1/4", "1/8", "1/8 T", "1/16", "1/16 T", "1/32" };
    return names;
}

const juce::StringArray& delayTimeNames()
{
    static const juce::StringArray names { "1/2", "1/4 D", "1/4", "1/8 D", "1/8", "1/8 T", "1/16" };
    return names;
}

double delayTimeInBeats (int choiceIndex)
{
    static const double beats[] = { 2.0, 1.5, 1.0, 0.75, 0.5, 1.0 / 3.0, 0.25 };
    return beats[juce::jlimit (0, 6, choiceIndex)];
}

juce::AudioProcessorValueTreeState::ParameterLayout createStackParameterLayout()
{
    using namespace GlobalParam;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int l = 0; l < numLayers; ++l)
    {
        const auto name = [l] (const char* what) { return "L" + juce::String (l + 1) + " " + what; };
        const auto id = [l] (const char* suffix) { return layerId (l, suffix); };
        auto cutoffRange = Range (20.0f, 20000.0f);
        cutoffRange.setSkewForCentre (1000.0f);

        layout.add (std::make_unique<APB> (pid (id (LayerParam::on)), name ("On"), l == 0),
                    std::make_unique<APC> (pid (id (LayerParam::wave)), name ("Wave"), waveNames(), 0),
                    std::make_unique<API> (pid (id (LayerParam::octave)), name ("Octave"), -3, 3, 0),
                    std::make_unique<API> (pid (id (LayerParam::semitone)), name ("Semitone"), -12, 12, 0),
                    std::make_unique<APF> (pid (id (LayerParam::fine)), name ("Fine"), Range (-100.0f, 100.0f), 0.0f,
                                           juce::AudioParameterFloatAttributes().withLabel ("ct")),
                    unit (id (LayerParam::detune), name ("Detune"), 0.3f),
                    unit (id (LayerParam::width), name ("Width"), 0.6f),
                    unit (id (LayerParam::level), name ("Level"), 0.8f),
                    std::make_unique<APF> (pid (id (LayerParam::pan)), name ("Pan"), Range (-1.0f, 1.0f), 0.0f),
                    std::make_unique<APC> (pid (id (LayerParam::filterType)), name ("Filter"), filterTypeNames(), 1),
                    std::make_unique<APF> (pid (id (LayerParam::cutoff)), name ("Cutoff"), cutoffRange, 8000.0f,
                                           juce::AudioParameterFloatAttributes().withLabel ("Hz")),
                    unit (id (LayerParam::resonance), name ("Resonance"), 0.1f),
                    std::make_unique<APF> (pid (id (LayerParam::filterEnv)), name ("Filter Env"), Range (-1.0f, 1.0f), 0.0f),
                    unit (id (LayerParam::keyTrack), name ("Key Track"), 0.5f),
                    seconds (id (LayerParam::fAttack), name ("Filter Attack"), 0.005f),
                    seconds (id (LayerParam::fDecay), name ("Filter Decay"), 0.4f),
                    unit (id (LayerParam::fSustain), name ("Filter Sustain"), 0.3f),
                    seconds (id (LayerParam::fRelease), name ("Filter Release"), 0.3f),
                    seconds (id (LayerParam::aAttack), name ("Attack"), 0.005f),
                    seconds (id (LayerParam::aDecay), name ("Decay"), 0.3f),
                    unit (id (LayerParam::aSustain), name ("Sustain"), 0.8f),
                    seconds (id (LayerParam::aRelease), name ("Release"), 0.3f),
                    unit (id (LayerParam::velocity), name ("Velocity"), 0.5f),
                    std::make_unique<API> (pid (id (LayerParam::root)), name ("Sample Root"), 0, 127, 60),
                    std::make_unique<APB> (pid (id (LayerParam::loop)), name ("Sample Loop"), false));
    }

    layout.add (std::make_unique<APC> (pid (voiceMode), "Voice Mode", juce::StringArray { "Poly", "Mono" }, 0),
                std::make_unique<APF> (pid (glide), "Glide", Range (0.0f, 3.0f, 0.0f, 0.4f), 0.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("s")),
                std::make_unique<API> (pid (bendRange), "Bend Range", 0, 24, 2),
                std::make_unique<APC> (pid (lfo1Shape), "LFO 1 Shape",
                                       juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0),
                hertz (lfo1Rate, "LFO 1 Rate", 0.02f, 20.0f, 0.5f),
                std::make_unique<APF> (pid (lfo1Cutoff), "LFO 1 > Cutoff", Range (-1.0f, 1.0f), 0.0f),
                std::make_unique<APC> (pid (lfo2Shape), "LFO 2 Shape",
                                       juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0),
                hertz (lfo2Rate, "LFO 2 Rate", 0.02f, 20.0f, 5.5f),
                unit (lfo2Pitch, "LFO 2 > Pitch", 0.0f),
                unit (modWheelVibrato, "Mod Wheel > Vibrato", 0.5f),
                std::make_unique<APF> (pid (volume), "Volume", Range (-60.0f, 6.0f), -6.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<APB> (pid (arpOn), "Arp On", false),
                std::make_unique<APC> (pid (arpMode), "Arp Mode",
                                       juce::StringArray { "Up", "Down", "Up/Down", "Down/Up", "Random", "As Played", "Chord" }, 0),
                std::make_unique<APC> (pid (arpRate), "Arp Rate", rateNames(), 3),
                std::make_unique<API> (pid (arpOctaves), "Arp Octaves", 1, 4, 1),
                std::make_unique<APF> (pid (arpGate), "Arp Gate", Range (0.05f, 1.0f), 0.6f),
                unit (arpSwing, "Arp Swing", 0.0f),
                std::make_unique<APB> (pid (arpLatch), "Arp Latch", false),
                std::make_unique<API> (pid (arpSteps), "Arp Steps", 1, 16, 16));

    layout.add (std::make_unique<APB> (pid (gateOn), "Gate On", false),
                std::make_unique<APC> (pid (gateRate), "Gate Rate", rateNames(), 3),
                std::make_unique<API> (pid (gateSteps), "Gate Steps", 1, 16, 16),
                unit (gateDepth, "Gate Depth", 1.0f),
                std::make_unique<APF> (pid (gateAttack), "Gate Attack", Range (0.5f, 50.0f, 0.0f, 0.5f), 1.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("ms")),
                std::make_unique<APF> (pid (gateRelease), "Gate Release", Range (1.0f, 200.0f, 0.0f, 0.5f), 12.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<APB> (pid (distOn), "Distortion On", false),
                unit (distDrive, "Distortion Drive", 0.4f),
                unit (distTone, "Distortion Tone", 0.6f),
                unit (distMix, "Distortion Mix", 1.0f),
                std::make_unique<APB> (pid (chorusOn), "Chorus On", false),
                unit (chorusRate, "Chorus Rate", 0.3f),
                unit (chorusDepth, "Chorus Depth", 0.4f),
                unit (chorusMix, "Chorus Mix", 0.5f),
                std::make_unique<APB> (pid (phaserOn), "Phaser On", false),
                unit (phaserRate, "Phaser Rate", 0.3f),
                unit (phaserDepth, "Phaser Depth", 0.6f),
                unit (phaserFeedback, "Phaser Feedback", 0.4f),
                unit (phaserMix, "Phaser Mix", 0.5f),
                std::make_unique<APB> (pid (delayOn), "Delay On", false),
                std::make_unique<APC> (pid (delayTime), "Delay Time", delayTimeNames(), 3),
                unit (delayFeedback, "Delay Feedback", 0.35f),
                unit (delayMix, "Delay Mix", 0.25f),
                std::make_unique<APB> (pid (delayPingPong), "Delay Ping-Pong", true),
                std::make_unique<APB> (pid (reverbOn), "Reverb On", false),
                unit (reverbSize, "Reverb Size", 0.6f),
                unit (reverbDamping, "Reverb Damping", 0.5f),
                unit (reverbWidth, "Reverb Width", 1.0f),
                unit (reverbMix, "Reverb Mix", 0.25f));

    return layout;
}

StackParameterReader::StackParameterReader (juce::AudioProcessorValueTreeState& state)
{
    for (auto* param : state.processor.getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
            ids.push_back (withId->paramID.toStdString());

    // ids owns the strings, so the string_view keys stay valid.
    for (const auto& id : ids)
        values[id] = state.getRawParameterValue (juce::String (id));

    for (int l = 0; l < numLayers; ++l)
        for (size_t i = 0; i < 25; ++i)
            layerValues[(size_t) l][i] = state.getRawParameterValue (layerId (l, layerSuffixes[i]));
}

float StackParameterReader::get (const char* id) const
{
    const auto it = values.find (id);
    jassert (it != values.end() && it->second != nullptr);
    return it->second->load();
}

void StackParameterReader::read (StackParams& p, FxSettings& fx) const
{
    using namespace GlobalParam;

    for (size_t l = 0; l < (size_t) numLayers; ++l)
    {
        const auto& v = layerValues[l];
        auto f = [&v] (size_t i) { return v[i]->load(); };
        auto& s = p.layers[l];
        s.enabled = f (0) > 0.5f;
        s.wave = (LayerWave) (int) f (1);
        s.octave = (int) f (2);
        s.semitone = (int) f (3);
        s.fineCents = f (4);
        s.detune = f (5);
        s.width = f (6);
        s.level = f (7);
        s.pan = f (8);
        s.filterType = (FilterType) (int) f (9);
        s.cutoffHz = f (10);
        s.resonance = f (11);
        s.filterEnvAmount = f (12);
        s.keyTrack = f (13);
        s.filterEnv = { f (14), f (15), f (16), f (17) };
        s.ampEnv = { f (18), f (19), f (20), f (21) };
        s.velocity = f (22);
        s.sampleRoot = (int) f (23);
        s.sampleLoop = f (24) > 0.5f;
    }

    p.voiceMode = (VoiceMode) (int) get (voiceMode);
    p.glideSeconds = get (glide);
    p.bendRangeSemitones = get (bendRange);
    p.lfo1Shape = (LfoShape) (int) get (lfo1Shape);
    p.lfo1RateHz = get (lfo1Rate);
    p.lfo1ToCutoff = get (lfo1Cutoff);
    p.lfo2Shape = (LfoShape) (int) get (lfo2Shape);
    p.lfo2RateHz = get (lfo2Rate);
    p.lfo2ToPitch = get (lfo2Pitch);
    p.modWheelToVibrato = get (modWheelVibrato);
    p.volumeGain = juce::Decibels::decibelsToGain (get (volume), -60.0f);

    p.arp.enabled = get (arpOn) > 0.5f;
    p.arp.mode = (ArpMode) (int) get (arpMode);
    p.arp.rate = (NoteRate) (int) get (arpRate);
    p.arp.octaves = (int) get (arpOctaves);
    p.arp.gate = get (arpGate);
    p.arp.swing = get (arpSwing);
    p.arp.latch = get (arpLatch) > 0.5f;
    p.arp.steps = (int) get (arpSteps);

    p.gate.enabled = get (gateOn) > 0.5f;
    p.gate.rate = (NoteRate) (int) get (gateRate);
    p.gate.steps = (int) get (gateSteps);
    p.gate.depth = get (gateDepth);
    p.gate.attackMs = get (gateAttack);
    p.gate.releaseMs = get (gateRelease);

    fx.distOn = get (distOn) > 0.5f;
    fx.distDrive = get (distDrive);
    fx.distTone = get (distTone);
    fx.distMix = get (distMix);
    fx.chorusOn = get (chorusOn) > 0.5f;
    fx.chorusRate = get (chorusRate);
    fx.chorusDepth = get (chorusDepth);
    fx.chorusMix = get (chorusMix);
    fx.phaserOn = get (phaserOn) > 0.5f;
    fx.phaserRate = get (phaserRate);
    fx.phaserDepth = get (phaserDepth);
    fx.phaserFeedback = get (phaserFeedback);
    fx.phaserMix = get (phaserMix);
    fx.delayOn = get (delayOn) > 0.5f;
    fx.delayBeats = delayTimeInBeats ((int) get (delayTime));
    fx.delayFeedback = get (delayFeedback);
    fx.delayMix = get (delayMix);
    fx.delayPingPong = get (delayPingPong) > 0.5f;
    fx.reverbOn = get (reverbOn) > 0.5f;
    fx.reverbSize = get (reverbSize);
    fx.reverbDamping = get (reverbDamping);
    fx.reverbWidth = get (reverbWidth);
    fx.reverbMix = get (reverbMix);
}

} // namespace stack
