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
    const int block = juce::jmax (samplesPerBlock, 32);
    instrument.prepare (sampleRate, block);
    instrument.setParams (reader.read());
    leftBuffer.assign ((size_t) block, 0.0f);
    rightBuffer.assign ((size_t) block, 0.0f);
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
    instrument.setParams (reader.read());
    readTransport();

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

    gateStepPlaying.store (instrument.currentGateStep(), std::memory_order_relaxed);
}

void LadderSynthProcessor::readTransport()
{
    double bpm = 120.0, ppq = 0.0;
    bool playing = false;

    if (auto* host = getPlayHead())
    {
        if (const auto position = host->getPosition())
        {
            if (const auto hostBpm = position->getBpm())
                bpm = *hostBpm;
            if (const auto hostPpq = position->getPpqPosition())
            {
                ppq = *hostPpq;
                playing = position->getIsPlaying();
            }
        }
    }

    instrument.setTransport (bpm, playing, ppq);
}

void LadderSynthProcessor::renderRange (juce::AudioBuffer<float>& buffer, int start, int numSamples)
{
    const int channels = buffer.getNumChannels();
    while (numSamples > 0)
    {
        const int chunk = juce::jmin (numSamples, (int) leftBuffer.size());
        instrument.render (leftBuffer.data(), rightBuffer.data(), chunk);

        if (channels >= 2)
        {
            buffer.copyFrom (0, start, leftBuffer.data(), chunk);
            buffer.copyFrom (1, start, rightBuffer.data(), chunk);
        }
        else if (channels == 1)
        {
            buffer.copyFrom (0, start, leftBuffer.data(), chunk, 0.5f);
            buffer.addFrom (0, start, rightBuffer.data(), chunk, 0.5f);
        }
        start += chunk;
        numSamples -= chunk;
    }
}

void LadderSynthProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        instrument.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        instrument.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())
        instrument.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isControllerOfType (1))
        instrument.setModWheel ((float) m.getControllerValue() / 127.0f);
    else if (m.isSustainPedalOn())
        instrument.setSustainPedal (true);
    else if (m.isSustainPedalOff())
        instrument.setSustainPedal (false);
    else if (m.isAllSoundOff())
        instrument.reset();
    else if (m.isAllNotesOff())
        instrument.allNotesOff();
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
