#include "Parameters.h"

namespace
{
using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;
using Range = juce::NormalisableRange<float>;

const juce::StringArray rangeNames { "Lo", "32'", "16'", "8'", "4'", "2'" };
const juce::StringArray waveNames { "Triangle", "Tri-Saw", "Rev Saw", "Saw", "Square", "Wide Pulse", "Narrow Pulse" };

juce::ParameterID pid (const char* id) { return { id, 1 }; }
// Parameters added after the first release carry version 2, so hosts keep their
// automation for the original ones.
juce::ParameterID pid2 (const char* id) { return { id, 2 }; }

std::unique_ptr<APF> level2 (const char* id, const char* name, float def)
{
    return std::make_unique<APF> (pid2 (id), name, Range (0.0f, 1.0f), def);
}

std::unique_ptr<APF> percent2 (const char* id, const char* name, float lo, float hi, float def)
{
    return std::make_unique<APF> (pid2 (id), name, Range (lo, hi), def,
                                  juce::AudioParameterFloatAttributes()
                                      .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; })
                                      .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; }));
}

// Subsets of the note lengths, as (choice index -> NoteLength) tables.
const int arpRates[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11 };      // 1/4 .. 1/32
const int gateRates[] = { 6, 7, 9, 10, 11 };                 // 1/8, 1/8T, 1/16, 1/16T, 1/32
const int delayTimes[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };  // 1/2 .. 1/16T

template <size_t N>
juce::StringArray namesFor (const int (&table)[N])
{
    juce::StringArray names;
    for (int index : table)
        names.add (noteLengthNames()[index]);
    return names;
}

template <size_t N>
ladder::NoteLength lengthFor (const int (&table)[N], float choice)
{
    const int i = juce::jlimit (0, (int) N - 1, (int) choice);
    return (ladder::NoteLength) table[i];
}

template <size_t N>
int choiceFor (const int (&table)[N], int noteLength)
{
    for (size_t i = 0; i < N; ++i)
        if (table[i] == noteLength)
            return (int) i;
    return 0;
}

Range timeRange() { return Range (0.001f, 10.0f, 0.0f, 0.25f); }

std::unique_ptr<APF> level (const char* id, const char* name, float def)
{
    return std::make_unique<APF> (pid (id), name, Range (0.0f, 1.0f), def);
}

std::unique_ptr<APF> seconds (const char* id, const char* name, float def)
{
    return std::make_unique<APF> (pid (id), name, timeRange(), def,
                                  juce::AudioParameterFloatAttributes().withLabel ("s"));
}

std::unique_ptr<APF> semitones (const char* id, const char* name)
{
    return std::make_unique<APF> (pid (id), name, Range (-7.0f, 7.0f, 0.0f), 0.0f,
                                  juce::AudioParameterFloatAttributes().withLabel ("st"));
}
} // namespace

const juce::StringArray& noteLengthNames()
{
    static const juce::StringArray names { "1/1", "1/2", "1/4D", "1/4", "1/4T", "1/8D", "1/8",
                                           "1/8T", "1/16D", "1/16", "1/16T", "1/32" };
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace ParamIDs;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<APC> (pid (osc1Range), "Osc 1 Range", rangeNames, 3),
                std::make_unique<APC> (pid (osc1Wave), "Osc 1 Wave", waveNames, 3),
                std::make_unique<APC> (pid (osc2Range), "Osc 2 Range", rangeNames, 3),
                std::make_unique<APC> (pid (osc2Wave), "Osc 2 Wave", waveNames, 3),
                semitones (osc2Tune, "Osc 2 Tune"),
                std::make_unique<APC> (pid (osc3Range), "Osc 3 Range", rangeNames, 3),
                std::make_unique<APC> (pid (osc3Wave), "Osc 3 Wave", waveNames, 0),
                semitones (osc3Tune, "Osc 3 Tune"),
                std::make_unique<APB> (pid (osc3KeyTrack), "Osc 3 Keyboard", true));

    layout.add (level (mixOsc1, "Osc 1 Level", 1.0f),
                level (mixOsc2, "Osc 2 Level", 0.0f),
                level (mixOsc3, "Osc 3 Level", 0.0f),
                level (mixNoise, "Noise Level", 0.0f),
                std::make_unique<APC> (pid (noiseColour), "Noise Colour", juce::StringArray { "White", "Pink" }, 0));

    auto cutoffRange = Range (20.0f, 20000.0f);
    cutoffRange.setSkewForCentre (1000.0f);
    layout.add (std::make_unique<APF> (pid (cutoff), "Cutoff", cutoffRange, 2000.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("Hz")),
                level (emphasis, "Emphasis", 0.3f),
                level (contour, "Contour", 0.5f),
                level (drive, "Drive", 0.2f),
                std::make_unique<APC> (pid (keyTrack), "Key Track", juce::StringArray { "Off", "1/3", "2/3", "Full" }, 1));

    layout.add (seconds (fAttack, "Filter Attack", 0.005f),
                seconds (fDecay, "Filter Decay", 0.4f),
                level (fSustain, "Filter Sustain", 0.3f),
                seconds (fRelease, "Filter Release", 0.3f),
                seconds (aAttack, "Amp Attack", 0.003f),
                seconds (aDecay, "Amp Decay", 0.3f),
                level (aSustain, "Amp Sustain", 0.8f),
                seconds (aRelease, "Amp Release", 0.25f));

    layout.add (std::make_unique<APF> (pid (glide), "Glide", Range (0.0f, 3.0f, 0.0f, 0.4f), 0.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("s")),
                std::make_unique<APC> (pid (voiceMode), "Voice Mode", juce::StringArray { "Mono", "Poly" }, 0),
                std::make_unique<APB> (pid (legato), "Legato", true),
                level (modMix, "Mod Mix", 0.0f),
                std::make_unique<APB> (pid (oscMod), "Osc Modulation", false),
                std::make_unique<APB> (pid (filterMod), "Filter Modulation", false),
                level (modAmount, "Mod Amount", 0.0f));

    layout.add (level (velAmp, "Velocity to Amp", 0.5f),
                level (velFilter, "Velocity to Filter", 0.0f),
                std::make_unique<juce::AudioParameterInt> (pid (bendRange), "Bend Range", 0, 12, 2),
                std::make_unique<APF> (pid (masterTune), "Tune", Range (-100.0f, 100.0f), 0.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("ct")),
                std::make_unique<APF> (pid (volume), "Volume", Range (-60.0f, 6.0f), -6.0f,
                                       juce::AudioParameterFloatAttributes().withLabel ("dB")));

    using ladder::NoteLength;
    const ladder::ArpSettings arpDefaults;
    const ladder::GateSettings gateDefaults;
    const ladder::ChorusSettings chorusDefaults;
    const ladder::DelaySettings delayDefaults;
    const ladder::ReverbSettings reverbDefaults;

    layout.add (std::make_unique<APB> (pid2 (arpOn), "Arp On", false),
                std::make_unique<APC> (pid2 (arpMode), "Arp Mode",
                                       juce::StringArray { "Up", "Down", "Up/Down", "Random", "As Played", "Chord" }, 0),
                std::make_unique<APC> (pid2 (arpRate), "Arp Rate", namesFor (arpRates),
                                       choiceFor (arpRates, (int) arpDefaults.rate)),
                std::make_unique<juce::AudioParameterInt> (pid2 (arpOctaves), "Arp Octaves", 1, 4, 1),
                percent2 (arpGate, "Arp Gate", 0.05f, 1.0f, arpDefaults.gate),
                percent2 (arpSwing, "Arp Swing", 0.0f, 1.0f, 0.0f),
                std::make_unique<APB> (pid2 (arpHold), "Arp Hold", false));

    layout.add (std::make_unique<APB> (pid2 (gateOn), "Gate On", false),
                std::make_unique<APC> (pid2 (gateRate), "Gate Rate", namesFor (gateRates),
                                       choiceFor (gateRates, (int) gateDefaults.rate)),
                percent2 (gateDepth, "Gate Depth", 0.0f, 1.0f, gateDefaults.depth),
                percent2 (gateLength, "Gate Length", 0.1f, 1.0f, gateDefaults.length),
                percent2 (gateSmooth, "Gate Smooth", 0.0f, 1.0f, gateDefaults.smooth));
    for (int i = 0; i < ladder::GateSettings::numSteps; ++i)
        layout.add (std::make_unique<APB> (pid2 (gateSteps[i]), "Gate Step " + juce::String (i + 1), gateDefaults.steps[i]));

    auto chorusRateRange = Range (0.05f, 8.0f);
    chorusRateRange.setSkewForCentre (1.0f);
    layout.add (std::make_unique<APB> (pid2 (chorusOn), "Chorus On", false),
                std::make_unique<APF> (pid2 (chorusRate), "Chorus Rate", chorusRateRange, chorusDefaults.rateHz,
                                       juce::AudioParameterFloatAttributes().withLabel ("Hz")),
                level2 (chorusDepth, "Chorus Depth", chorusDefaults.depth),
                level2 (chorusMix, "Chorus Mix", chorusDefaults.mix));

    layout.add (std::make_unique<APB> (pid2 (delayOn), "Delay On", false),
                std::make_unique<APC> (pid2 (delayTime), "Delay Time", namesFor (delayTimes),
                                       choiceFor (delayTimes, (int) delayDefaults.time)),
                percent2 (delayFeedback, "Delay Feedback", 0.0f, 0.9f, delayDefaults.feedback),
                level2 (delayDamp, "Delay Damping", delayDefaults.damping),
                level2 (delayMix, "Delay Mix", delayDefaults.mix),
                std::make_unique<APB> (pid2 (delayPingPong), "Delay Ping-Pong", delayDefaults.pingPong));

    layout.add (std::make_unique<APB> (pid2 (reverbOn), "Reverb On", false),
                level2 (reverbSize, "Reverb Size", reverbDefaults.size),
                level2 (reverbDamp, "Reverb Damping", reverbDefaults.damping),
                level2 (reverbWidth, "Reverb Width", reverbDefaults.width),
                level2 (reverbMix, "Reverb Mix", reverbDefaults.mix));

    return layout;
}

ParameterReader::ParameterReader (juce::AudioProcessorValueTreeState& state)
{
    for (auto* param : state.processor.getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (param))
            ids.push_back (withId->paramID.toStdString());

    // ids owns the strings, so the string_view keys stay valid.
    for (const auto& id : ids)
        values[id] = state.getRawParameterValue (juce::String (id));
}

float ParameterReader::get (const char* id) const
{
    const auto it = values.find (id);
    jassert (it != values.end() && it->second != nullptr);
    return it->second->load();
}

ladder::SynthParams ParameterReader::read() const
{
    using namespace ParamIDs;
    ladder::SynthParams p;

    const char* ranges[] = { osc1Range, osc2Range, osc3Range };
    const char* waves[] = { osc1Wave, osc2Wave, osc3Wave };
    for (int i = 0; i < 3; ++i)
    {
        p.osc[i].range = (ladder::Range) (int) get (ranges[i]);
        p.osc[i].wave = (ladder::Waveform) (int) get (waves[i]);
    }
    p.osc[1].tuneSemitones = get (osc2Tune);
    p.osc[2].tuneSemitones = get (osc3Tune);
    p.osc3KeyTrack = get (osc3KeyTrack) > 0.5f;

    p.mixOsc[0] = get (mixOsc1);
    p.mixOsc[1] = get (mixOsc2);
    p.mixOsc[2] = get (mixOsc3);
    p.mixNoise = get (mixNoise);
    p.noiseColour = (ladder::NoiseColour) (int) get (noiseColour);

    p.cutoffHz = get (cutoff);
    p.emphasis = get (emphasis);
    p.contour = get (contour);
    p.drive = get (drive);
    p.keyTrack = (ladder::KeyTrack) (int) get (keyTrack);

    p.filterEnv = { get (fAttack), get (fDecay), get (fSustain), get (fRelease) };
    p.ampEnv = { get (aAttack), get (aDecay), get (aSustain), get (aRelease) };

    p.glideSeconds = get (glide);
    p.voiceMode = (ladder::VoiceMode) (int) get (voiceMode);
    p.legato = get (legato) > 0.5f;
    p.modMix = get (modMix);
    p.oscMod = get (oscMod) > 0.5f;
    p.filterMod = get (filterMod) > 0.5f;
    p.modAmount = get (modAmount);

    p.velocityToAmp = get (velAmp);
    p.velocityToFilter = get (velFilter);
    p.bendRangeSemitones = get (bendRange);
    p.masterTuneCents = get (masterTune);
    p.volumeGain = juce::Decibels::decibelsToGain (get (volume), -60.0f);

    p.arp.enabled = get (arpOn) > 0.5f;
    p.arp.mode = (ladder::ArpMode) (int) get (arpMode);
    p.arp.rate = lengthFor (arpRates, get (arpRate));
    p.arp.octaves = (int) get (arpOctaves);
    p.arp.gate = get (arpGate);
    p.arp.swing = get (arpSwing);
    p.arp.hold = get (arpHold) > 0.5f;

    p.gate.enabled = get (gateOn) > 0.5f;
    p.gate.rate = lengthFor (gateRates, get (gateRate));
    p.gate.depth = get (gateDepth);
    p.gate.length = get (gateLength);
    p.gate.smooth = get (gateSmooth);
    for (int i = 0; i < ladder::GateSettings::numSteps; ++i)
        p.gate.steps[i] = get (gateSteps[i]) > 0.5f;

    p.chorus = { get (chorusOn) > 0.5f, get (chorusRate), get (chorusDepth), get (chorusMix) };
    p.delay = { get (delayOn) > 0.5f, lengthFor (delayTimes, get (delayTime)), get (delayFeedback),
                get (delayDamp), get (delayMix), get (delayPingPong) > 0.5f };
    p.reverb = { get (reverbOn) > 0.5f, get (reverbSize), get (reverbDamp), get (reverbWidth), get (reverbMix) };
    return p;
}
