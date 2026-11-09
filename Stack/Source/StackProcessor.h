#pragma once

#include "StackParameters.h"
#include "StackPresets.h"
#include "engine/StackEngine.h"
#include "fx/FxChain.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include <vector>

class LayerStackProcessor : public juce::AudioProcessor,
                            private juce::Timer
{
public:
    LayerStackProcessor();
    ~LayerStackProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- used by the editor (message thread) ----
    juce::AudioProcessorValueTreeState apvts;

    bool getArpStepOn (int step) const;
    float getArpStepVelocity (int step) const;
    void setArpStep (int step, bool on, float velocity);
    bool getGateStepOn (int step) const;
    void setGateStep (int step, bool on);

    std::array<int, stack::numFx> getFxOrder() const;
    void moveFx (int position, int delta);   // swap the effect at position with its neighbour

    bool loadSample (int layer, const juce::File& file, juce::String& error);
    void clearSample (int layer);
    juce::String getSampleName (int layer) const;

    void loadFactoryPreset (int index);
    bool saveUserPreset (const juce::String& name, const juce::String& category, juce::String& error);
    bool loadUserPreset (const juce::File& file);
    juce::String getPresetName() const { return presetName; }

    // Bumped whenever a preset or state load changes things the editor caches.
    std::atomic<int> stateVersion { 0 };

private:
    stack::StackParameterReader reader;
    stack::StackEngine engine;
    stack::FxChain fx;
    stack::StackParams engineParams;
    stack::FxSettings fxSettings;
    juce::AudioBuffer<float> work;
    std::vector<stack::InputEvent> inputEvents;
    juce::AudioFormatManager formats;

    // Patterns and effect order: written by the editor, read by the audio thread.
    std::atomic<juce::uint32> arpStepMask { 0xffff };
    std::array<std::atomic<float>, stack::maxSteps> arpStepVelocity;
    std::atomic<juce::uint32> gateStepMask { 0x5555 };
    std::atomic<juce::uint32> fxOrderPacked { 0 };

    // Samples. The editor thread loads a file into a new SampleData and publishes
    // its pointer; the audio thread picks it up at the start of the next block.
    // Replaced samples wait in a graveyard until the audio thread has moved on.
    std::array<std::shared_ptr<stack::SampleData>, stack::numLayers> loadedSamples;
    std::array<std::atomic<const stack::SampleData*>, stack::numLayers> publishedSamples {};
    struct Retired { std::shared_ptr<stack::SampleData> sample; juce::uint64 generation; };
    std::vector<Retired> graveyard;
    std::atomic<juce::uint64> publishGeneration { 0 };
    std::atomic<juce::uint64> audioSeenGeneration { 0 };

    int currentProgram = 0;
    juce::String presetName { "Init" };

    void timerCallback() override;
    void publishSample (int layer, std::shared_ptr<stack::SampleData> sample);
    void readPatterns();
    void applyPatternStrings (const juce::String& arp, const juce::String& gate);

    juce::ValueTree buildState();
    void restoreState (const juce::ValueTree& state);
    static juce::uint32 packOrder (const std::array<int, stack::numFx>& order);
    static std::array<int, stack::numFx> unpackOrder (juce::uint32 packed);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LayerStackProcessor)
};
