#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <utility>
#include <vector>

// Factory patches. Each one starts from the default value of every parameter
// and overrides only what it lists, so presets never inherit leftovers.
struct Preset
{
    const char* category;
    const char* name;
    std::vector<std::pair<const char*, float>> values; // plain (not normalised) values
};

const std::vector<Preset>& factoryPresets();

// Categories in the order the browser shows them.
const std::vector<const char*>& presetCategories();

void applyPreset (const Preset& preset, juce::AudioProcessorValueTreeState& apvts);
