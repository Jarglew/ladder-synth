#pragma once

#include "engine/StackParams.h"
#include "fx/FxChain.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace stack
{

// Per-layer parameter IDs are "l1" .. "l4" followed by one of these suffixes.
namespace LayerParam
{
inline constexpr const char* on = "On";
inline constexpr const char* wave = "Wave";
inline constexpr const char* octave = "Oct";
inline constexpr const char* semitone = "Semi";
inline constexpr const char* fine = "Fine";
inline constexpr const char* detune = "Detune";
inline constexpr const char* width = "Width";
inline constexpr const char* level = "Level";
inline constexpr const char* pan = "Pan";
inline constexpr const char* filterType = "FType";
inline constexpr const char* cutoff = "Cutoff";
inline constexpr const char* resonance = "Reso";
inline constexpr const char* filterEnv = "FEnv";
inline constexpr const char* keyTrack = "KeyTrk";
inline constexpr const char* fAttack = "FA";
inline constexpr const char* fDecay = "FD";
inline constexpr const char* fSustain = "FS";
inline constexpr const char* fRelease = "FR";
inline constexpr const char* aAttack = "AA";
inline constexpr const char* aDecay = "AD";
inline constexpr const char* aSustain = "AS";
inline constexpr const char* aRelease = "AR";
inline constexpr const char* velocity = "Vel";
inline constexpr const char* root = "Root";
inline constexpr const char* loop = "Loop";
} // namespace LayerParam

inline juce::String layerId (int layer, const char* suffix) { return "l" + juce::String (layer + 1) + suffix; }

namespace GlobalParam
{
inline constexpr const char* voiceMode = "voiceMode";
inline constexpr const char* glide = "glide";
inline constexpr const char* bendRange = "bendRange";
inline constexpr const char* lfo1Shape = "lfo1Shape";
inline constexpr const char* lfo1Rate = "lfo1Rate";
inline constexpr const char* lfo1Cutoff = "lfo1Cutoff";
inline constexpr const char* lfo2Shape = "lfo2Shape";
inline constexpr const char* lfo2Rate = "lfo2Rate";
inline constexpr const char* lfo2Pitch = "lfo2Pitch";
inline constexpr const char* modWheelVibrato = "modWheelVib";
inline constexpr const char* volume = "volume";

inline constexpr const char* arpOn = "arpOn";
inline constexpr const char* arpMode = "arpMode";
inline constexpr const char* arpRate = "arpRate";
inline constexpr const char* arpOctaves = "arpOct";
inline constexpr const char* arpGate = "arpGate";
inline constexpr const char* arpSwing = "arpSwing";
inline constexpr const char* arpLatch = "arpLatch";
inline constexpr const char* arpSteps = "arpSteps";

inline constexpr const char* gateOn = "gateOn";
inline constexpr const char* gateRate = "gateRate";
inline constexpr const char* gateSteps = "gateSteps";
inline constexpr const char* gateDepth = "gateDepth";
inline constexpr const char* gateAttack = "gateAttack";
inline constexpr const char* gateRelease = "gateRelease";

inline constexpr const char* distOn = "distOn";
inline constexpr const char* distDrive = "distDrive";
inline constexpr const char* distTone = "distTone";
inline constexpr const char* distMix = "distMix";
inline constexpr const char* chorusOn = "chorOn";
inline constexpr const char* chorusRate = "chorRate";
inline constexpr const char* chorusDepth = "chorDepth";
inline constexpr const char* chorusMix = "chorMix";
inline constexpr const char* phaserOn = "phasOn";
inline constexpr const char* phaserRate = "phasRate";
inline constexpr const char* phaserDepth = "phasDepth";
inline constexpr const char* phaserFeedback = "phasFb";
inline constexpr const char* phaserMix = "phasMix";
inline constexpr const char* delayOn = "delOn";
inline constexpr const char* delayTime = "delTime";
inline constexpr const char* delayFeedback = "delFb";
inline constexpr const char* delayMix = "delMix";
inline constexpr const char* delayPingPong = "delPing";
inline constexpr const char* reverbOn = "revOn";
inline constexpr const char* reverbSize = "revSize";
inline constexpr const char* reverbDamping = "revDamp";
inline constexpr const char* reverbWidth = "revWidth";
inline constexpr const char* reverbMix = "revMix";
} // namespace GlobalParam

const juce::StringArray& waveNames();
const juce::StringArray& filterTypeNames();
const juce::StringArray& rateNames();
const juce::StringArray& delayTimeNames();
double delayTimeInBeats (int choiceIndex);

juce::AudioProcessorValueTreeState::ParameterLayout createStackParameterLayout();

// Reads automatable parameters into the engine and effects settings.
// Pointers are looked up once, so the audio thread never allocates.
class StackParameterReader
{
public:
    explicit StackParameterReader (juce::AudioProcessorValueTreeState& state);
    void read (StackParams& engine, FxSettings& fx) const;

private:
    std::vector<std::string> ids;
    std::unordered_map<std::string_view, std::atomic<float>*> values;
    std::array<std::array<std::atomic<float>*, 25>, numLayers> layerValues {};
    float get (const char* id) const;
};

} // namespace stack
