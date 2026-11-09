#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <utility>
#include <vector>

namespace stack
{

// A factory patch: overrides on top of every parameter's default value, plus
// optional arpeggiator and gate patterns written as strings:
//   'X' = accented step, 'x' = normal step, '.' = rest (arp) / closed (gate).
struct FactoryPreset
{
    juce::String name;
    juce::String category;
    std::vector<std::pair<juce::String, float>> values;
    juce::String arpPattern;    // empty = all steps on, full velocity
    juce::String gatePattern;   // empty = default alternating pattern
};

const std::vector<FactoryPreset>& factoryPresets();
juce::StringArray presetCategories();   // in browser order

// User presets live in ~/Music/Layer Stack/Presets as .lspreset files.
juce::File userPresetFolder();

struct UserPresetInfo
{
    juce::File file;
    juce::String name;
    juce::String category;
};

std::vector<UserPresetInfo> scanUserPresets();

} // namespace stack
