#pragma once

#include "dsp/SynthParams.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <string>

namespace ParamIDs
{
inline constexpr const char* osc1Range = "osc1Range";
inline constexpr const char* osc1Wave = "osc1Wave";
inline constexpr const char* osc2Range = "osc2Range";
inline constexpr const char* osc2Wave = "osc2Wave";
inline constexpr const char* osc2Tune = "osc2Tune";
inline constexpr const char* osc3Range = "osc3Range";
inline constexpr const char* osc3Wave = "osc3Wave";
inline constexpr const char* osc3Tune = "osc3Tune";
inline constexpr const char* osc3KeyTrack = "osc3KeyTrack";

inline constexpr const char* mixOsc1 = "mixOsc1";
inline constexpr const char* mixOsc2 = "mixOsc2";
inline constexpr const char* mixOsc3 = "mixOsc3";
inline constexpr const char* mixNoise = "mixNoise";
inline constexpr const char* noiseColour = "noiseColour";

inline constexpr const char* cutoff = "cutoff";
inline constexpr const char* emphasis = "emphasis";
inline constexpr const char* contour = "contour";
inline constexpr const char* drive = "drive";
inline constexpr const char* keyTrack = "keyTrack";

inline constexpr const char* fAttack = "fAttack";
inline constexpr const char* fDecay = "fDecay";
inline constexpr const char* fSustain = "fSustain";
inline constexpr const char* fRelease = "fRelease";
inline constexpr const char* aAttack = "aAttack";
inline constexpr const char* aDecay = "aDecay";
inline constexpr const char* aSustain = "aSustain";
inline constexpr const char* aRelease = "aRelease";

inline constexpr const char* glide = "glide";
inline constexpr const char* voiceMode = "voiceMode";
inline constexpr const char* legato = "legato";
inline constexpr const char* modMix = "modMix";
inline constexpr const char* oscMod = "oscMod";
inline constexpr const char* filterMod = "filterMod";
inline constexpr const char* modAmount = "modAmount";

inline constexpr const char* velAmp = "velAmp";
inline constexpr const char* velFilter = "velFilter";
inline constexpr const char* bendRange = "bendRange";
inline constexpr const char* masterTune = "masterTune";
inline constexpr const char* volume = "volume";

// Arpeggiator
inline constexpr const char* arpOn = "arpOn";
inline constexpr const char* arpMode = "arpMode";
inline constexpr const char* arpRate = "arpRate";
inline constexpr const char* arpOctaves = "arpOctaves";
inline constexpr const char* arpGate = "arpGate";
inline constexpr const char* arpSwing = "arpSwing";
inline constexpr const char* arpHold = "arpHold";

// Trance gate
inline constexpr const char* gateOn = "gateOn";
inline constexpr const char* gateRate = "gateRate";
inline constexpr const char* gateDepth = "gateDepth";
inline constexpr const char* gateLength = "gateLength";
inline constexpr const char* gateSmooth = "gateSmooth";
inline constexpr const char* gateSteps[16] = {
    "gateStep1", "gateStep2", "gateStep3", "gateStep4", "gateStep5", "gateStep6", "gateStep7", "gateStep8",
    "gateStep9", "gateStep10", "gateStep11", "gateStep12", "gateStep13", "gateStep14", "gateStep15", "gateStep16"
};

// Effects
inline constexpr const char* chorusOn = "chorusOn";
inline constexpr const char* chorusRate = "chorusRate";
inline constexpr const char* chorusDepth = "chorusDepth";
inline constexpr const char* chorusMix = "chorusMix";
inline constexpr const char* delayOn = "delayOn";
inline constexpr const char* delayTime = "delayTime";
inline constexpr const char* delayFeedback = "delayFeedback";
inline constexpr const char* delayDamp = "delayDamp";
inline constexpr const char* delayMix = "delayMix";
inline constexpr const char* delayPingPong = "delayPingPong";
inline constexpr const char* reverbOn = "reverbOn";
inline constexpr const char* reverbSize = "reverbSize";
inline constexpr const char* reverbDamp = "reverbDamp";
inline constexpr const char* reverbWidth = "reverbWidth";
inline constexpr const char* reverbMix = "reverbMix";
} // namespace ParamIDs

// Names of the tempo-synced note lengths, in ladder::NoteLength order.
const juce::StringArray& noteLengthNames();

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Reads the current parameter values into the engine's plain struct.
// The parameter pointers are looked up once, up front, so the audio thread
// never builds a juce::String (which would allocate) to find a parameter.
class ParameterReader
{
public:
    explicit ParameterReader (juce::AudioProcessorValueTreeState& state);
    ladder::SynthParams read() const;

private:
    std::vector<std::string> ids;
    std::unordered_map<std::string_view, std::atomic<float>*> values;
    float get (const char* id) const;
};
