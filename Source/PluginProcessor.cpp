#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

LadderSynthProcessor::LadderSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout()),
      reader (apvts)
{
}

void LadderSynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    engine.setParams (reader.read());
    monoBuffer.assign ((size_t) juce::jmax (samplesPerBlock, 1), 0.0f);
}

bool LadderSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void LadderSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    engine.setParams (reader.read());

    // Render between MIDI events so every note starts on its exact sample.
    int position = 0;
    for (const auto metadata : midi)
    {
        const int eventPos = juce::jlimit (0, buffer.getNumSamples(), metadata.samplePosition);
        renderRange (buffer, position, eventPos - position);
        handleMidi (metadata.getMessage());
        position = eventPos;
    }
    renderRange (buffer, position, buffer.getNumSamples() - position);
}

void LadderSynthProcessor::renderRange (juce::AudioBuffer<float>& buffer, int start, int numSamples)
{
    while (numSamples > 0)
    {
        const int chunk = juce::jmin (numSamples, (int) monoBuffer.size());
        engine.render (monoBuffer.data(), chunk);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, start, monoBuffer.data(), chunk);
        start += chunk;
        numSamples -= chunk;
    }
}

void LadderSynthProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())
        engine.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isControllerOfType (1))
        engine.setModWheel ((float) m.getControllerValue() / 127.0f);
    else if (m.isSustainPedalOn())
        engine.setSustainPedal (true);
    else if (m.isSustainPedalOff())
        engine.setSustainPedal (false);
    else if (m.isAllSoundOff())
        engine.reset();
    else if (m.isAllNotesOff())
        engine.allNotesOff();
}

int LadderSynthProcessor::getNumPrograms()
{
    return (int) factoryPresets().size();
}

void LadderSynthProcessor::setCurrentProgram (int index)
{
    const auto& presets = factoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return;
    currentProgram = index;
    applyPreset (presets[(size_t) index], apvts);
}

const juce::String LadderSynthProcessor::getProgramName (int index)
{
    const auto& presets = factoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return {};
    return presets[(size_t) index].name;
}

void LadderSynthProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void LadderSynthProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentProgram = state.getProperty ("program", 0);
            apvts.replaceState (state);
        }
    }
}

juce::AudioProcessorEditor* LadderSynthProcessor::createEditor()
{
    return new LadderSynthEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LadderSynthProcessor();
}
