#pragma once

#include <memory>
#include <string>
#include <vector>

namespace stack
{

// An audio file loaded into memory for a sample layer. Immutable once built:
// the message thread creates a new one and hands it over, so the audio thread
// never sees a half-written sample.
struct SampleData
{
    std::vector<float> left, right;   // right == left for mono files
    double sampleRate = 44100.0;
    std::string name;                 // shown in the editor
    std::string path;                 // saved with the patch so it can be reloaded

    size_t length() const { return left.size(); }
};

using SampleDataPtr = std::shared_ptr<const SampleData>;

} // namespace stack
