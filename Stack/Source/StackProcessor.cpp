#include "StackProcessor.h"
#include "StackEditor.h"

using namespace stack;

namespace
{
constexpr double maxSampleSeconds = 60.0;
const juce::Identifier extraTag ("Extra");
} // namespace

LayerStackProcessor::LayerStackProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createStackParameterLayout()),
      reader (apvts)
{
    for (auto& v : arpStepVelocity)
        v.store (1.0f);
    fxOrderPacked.store (packOrder ({ 0, 1, 2, 3, 4 }));
    formats.registerBasicFormats();
    inputEvents.reserve (1024);
    graveyard.reserve (16);
    startTimer (1000);
}

LayerStackProcessor::~LayerStackProcessor()
{
    stopTimer();
}

void LayerStackProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int block = juce::jmax (32, samplesPerBlock);
    engine.prepare (sampleRate, block);
    fx.prepare (sampleRate, block);
    work.setSize (2, block);
}

bool LayerStackProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void LayerStackProcessor::readPatterns()
{
    const auto arpMask = arpStepMask.load();
    const auto gateMask = gateStepMask.load();
    for (size_t i = 0; i < (size_t) maxSteps; ++i)
    {
        engineParams.arp.stepOn[i] = (arpMask >> i) & 1u;
        engineParams.arp.stepVelocity[i] = arpStepVelocity[i].load();
        engineParams.gate.stepOn[i] = (gateMask >> i) & 1u;
    }
    fxSettings.order = unpackOrder (fxOrderPacked.load());
}

void LayerStackProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    reader.read (engineParams, fxSettings);
    readPatterns();
    engine.setParams (engineParams);

    // Pick up newly loaded samples. Reading the generation first means that once
    // we report it back, we have definitely switched away from older samples.
    const auto generation = publishGeneration.load (std::memory_order_acquire);
    for (int l = 0; l < numLayers; ++l)
        engine.setSample (l, publishedSamples[(size_t) l].load (std::memory_order_acquire));
    audioSeenGeneration.store (generation, std::memory_order_release);

    Transport transport;
    if (auto* host = getPlayHead())
    {
        if (auto pos = host->getPosition())
        {
            if (auto bpm = pos->getBpm())
                transport.bpm = *bpm;
            transport.playing = pos->getIsPlaying();
            if (auto ppq = pos->getPpqPosition())
                transport.ppqAtBlockStart = *ppq;
            else
                transport.playing = false;
        }
    }

    inputEvents.clear();
    for (const auto metadata : midi)
    {
        if ((int) inputEvents.size() >= (int) inputEvents.capacity())
            break;   // never allocate on the audio thread
        const auto m = metadata.getMessage();
        const int at = juce::jlimit (0, juce::jmax (0, numSamples - 1), metadata.samplePosition);
        using T = InputEvent::Type;
        if (m.isNoteOn())
            inputEvents.push_back ({ T::noteOn, at, m.getNoteNumber(), m.getFloatVelocity() });
        else if (m.isNoteOff())
            inputEvents.push_back ({ T::noteOff, at, m.getNoteNumber(), 0.0f });
        else if (m.isPitchWheel())
            inputEvents.push_back ({ T::pitchBend, at, 0, (float) (m.getPitchWheelValue() - 8192) / 8192.0f });
        else if (m.isControllerOfType (1))
            inputEvents.push_back ({ T::modWheel, at, 0, (float) m.getControllerValue() / 127.0f });
        else if (m.isSustainPedalOn() || m.isSustainPedalOff())
            inputEvents.push_back ({ T::sustain, at, 0, m.isSustainPedalOn() ? 1.0f : 0.0f });
        else if (m.isAllSoundOff())
            inputEvents.push_back ({ T::allSoundOff, at, 0, 0.0f });
        else if (m.isAllNotesOff())
            inputEvents.push_back ({ T::allNotesOff, at, 0, 0.0f });
    }

    // Hosts may send bigger blocks than announced; render in pieces if so.
    buffer.clear();
    const int chunkSize = work.getNumSamples();
    if (chunkSize == 0)
        return;   // not prepared yet
    size_t nextEvent = 0;
    for (int start = 0; start < numSamples; start += chunkSize)
    {
        const int len = juce::jmin (chunkSize, numSamples - start);
        const size_t firstEvent = nextEvent;
        while (nextEvent < inputEvents.size() && inputEvents[nextEvent].offset < start + len)
        {
            inputEvents[nextEvent].offset -= start;
            ++nextEvent;
        }

        Transport t = transport;
        t.ppqAtBlockStart += start * (transport.bpm / 60.0 / getSampleRate());

        engine.process (work.getWritePointer (0), work.getWritePointer (1), len,
                        inputEvents.data() + firstEvent, (int) (nextEvent - firstEvent), t);

        juce::AudioBuffer<float> chunk (work.getArrayOfWritePointers(), 2, len);
        fx.process (chunk, fxSettings, transport.bpm);

        if (buffer.getNumChannels() >= 2)
        {
            buffer.copyFrom (0, start, work, 0, 0, len);
            buffer.copyFrom (1, start, work, 1, 0, len);
        }
        else
        {
            buffer.copyFrom (0, start, work, 0, 0, len);
            buffer.addFrom (0, start, work, 1, 0, len);
            buffer.applyGain (0, start, len, 0.5f);
        }
    }
}

// ---------------------------------------------------------------- patterns and fx order

bool LayerStackProcessor::getArpStepOn (int step) const { return (arpStepMask.load() >> step) & 1u; }
float LayerStackProcessor::getArpStepVelocity (int step) const { return arpStepVelocity[(size_t) step].load(); }

void LayerStackProcessor::setArpStep (int step, bool on, float velocity)
{
    if (step < 0 || step >= maxSteps)
        return;
    const auto bit = 1u << step;
    auto mask = arpStepMask.load();
    arpStepMask.store (on ? (mask | bit) : (mask & ~bit));
    arpStepVelocity[(size_t) step].store (juce::jlimit (0.05f, 1.0f, velocity));
}

bool LayerStackProcessor::getGateStepOn (int step) const { return (gateStepMask.load() >> step) & 1u; }

void LayerStackProcessor::setGateStep (int step, bool on)
{
    if (step < 0 || step >= maxSteps)
        return;
    const auto bit = 1u << step;
    auto mask = gateStepMask.load();
    gateStepMask.store (on ? (mask | bit) : (mask & ~bit));
}

juce::uint32 LayerStackProcessor::packOrder (const std::array<int, numFx>& order)
{
    juce::uint32 packed = 0;
    for (size_t i = 0; i < (size_t) numFx; ++i)
        packed |= (juce::uint32) (order[i] & 7) << (3 * i);
    return packed;
}

std::array<int, numFx> LayerStackProcessor::unpackOrder (juce::uint32 packed)
{
    std::array<int, numFx> order {};
    for (size_t i = 0; i < (size_t) numFx; ++i)
        order[i] = (int) ((packed >> (3 * i)) & 7u);

    // Repair anything that isn't a permutation of 0..4 (e.g. a damaged saved state).
    std::array<int, numFx> sorted = order;
    std::sort (sorted.begin(), sorted.end());
    for (size_t i = 0; i < (size_t) numFx; ++i)
        if (sorted[i] != (int) i)
            return { 0, 1, 2, 3, 4 };
    return order;
}

std::array<int, numFx> LayerStackProcessor::getFxOrder() const { return unpackOrder (fxOrderPacked.load()); }

void LayerStackProcessor::moveFx (int position, int delta)
{
    auto order = getFxOrder();
    const int other = position + delta;
    if (position < 0 || position >= numFx || other < 0 || other >= numFx)
        return;
    std::swap (order[(size_t) position], order[(size_t) other]);
    fxOrderPacked.store (packOrder (order));
}

// ---------------------------------------------------------------- samples

void LayerStackProcessor::publishSample (int layer, std::shared_ptr<SampleData> sample)
{
    auto old = std::move (loadedSamples[(size_t) layer]);
    loadedSamples[(size_t) layer] = std::move (sample);
    publishedSamples[(size_t) layer].store (loadedSamples[(size_t) layer].get(), std::memory_order_release);
    const auto generation = publishGeneration.fetch_add (1, std::memory_order_acq_rel) + 1;
    if (old != nullptr)
        graveyard.push_back ({ std::move (old), generation });
}

bool LayerStackProcessor::loadSample (int layer, const juce::File& file, juce::String& error)
{
    if (layer < 0 || layer >= numLayers)
        return false;

    std::unique_ptr<juce::AudioFormatReader> fileReader (formats.createReaderFor (file));
    if (fileReader == nullptr)
    {
        error = "Couldn't read \"" + file.getFileName() + "\". Use WAV, AIFF, FLAC or Ogg files.";
        return false;
    }

    const auto maxLength = (juce::int64) (fileReader->sampleRate * maxSampleSeconds);
    const auto length = (int) juce::jmin (fileReader->lengthInSamples, maxLength);
    if (length < 2)
    {
        error = "\"" + file.getFileName() + "\" is empty.";
        return false;
    }

    juce::AudioBuffer<float> audio ((int) juce::jmin (2u, fileReader->numChannels), length);
    fileReader->read (&audio, 0, length, 0, true, audio.getNumChannels() > 1);

    auto sample = std::make_shared<SampleData>();
    sample->sampleRate = fileReader->sampleRate;
    sample->name = file.getFileNameWithoutExtension().toStdString();
    sample->path = file.getFullPathName().toStdString();
    sample->left.assign (audio.getReadPointer (0), audio.getReadPointer (0) + length);
    const float* right = audio.getNumChannels() > 1 ? audio.getReadPointer (1) : audio.getReadPointer (0);
    sample->right.assign (right, right + length);

    publishSample (layer, std::move (sample));
    stateVersion++;
    return true;
}

void LayerStackProcessor::clearSample (int layer)
{
    if (layer >= 0 && layer < numLayers)
    {
        publishSample (layer, nullptr);
        stateVersion++;
    }
}

juce::String LayerStackProcessor::getSampleName (int layer) const
{
    const auto& s = loadedSamples[(size_t) layer];
    return s == nullptr ? juce::String() : juce::String (s->name);
}

void LayerStackProcessor::timerCallback()
{
    // Free replaced samples once the audio thread has switched away from them.
    const auto seen = audioSeenGeneration.load (std::memory_order_acquire);
    graveyard.erase (std::remove_if (graveyard.begin(), graveyard.end(),
                                     [seen] (const Retired& r) { return r.generation <= seen; }),
                     graveyard.end());
}

// ---------------------------------------------------------------- presets

int LayerStackProcessor::getNumPrograms() { return (int) factoryPresets().size(); }

const juce::String LayerStackProcessor::getProgramName (int index)
{
    const auto& presets = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) presets.size()) ? presets[(size_t) index].name : juce::String();
}

void LayerStackProcessor::setCurrentProgram (int index)
{
    loadFactoryPreset (index);
}

void LayerStackProcessor::applyPatternStrings (const juce::String& arp, const juce::String& gate)
{
    juce::uint32 arpMask = 0;
    for (int i = 0; i < maxSteps; ++i)
    {
        const auto c = arp.isEmpty() ? 'x' : (i < arp.length() ? (char) arp[i] : '.');
        if (c != '.')
            arpMask |= 1u << i;
        arpStepVelocity[(size_t) i].store (c == 'X' || arp.isEmpty() ? 1.0f : 0.6f);
    }
    arpStepMask.store (arpMask);

    if (gate.isEmpty())
    {
        gateStepMask.store (0x5555);
    }
    else
    {
        juce::uint32 gateMask = 0;
        for (int i = 0; i < maxSteps && i < gate.length(); ++i)
            if (gate[i] != '.')
                gateMask |= 1u << i;
        gateStepMask.store (gateMask);
    }
}

void LayerStackProcessor::loadFactoryPreset (int index)
{
    const auto& presets = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;
    const auto& preset = presets[(size_t) index];

    for (auto* param : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());

    for (const auto& [id, value] : preset.values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));

    applyPatternStrings (preset.arpPattern, preset.gatePattern);
    fxOrderPacked.store (packOrder ({ 0, 1, 2, 3, 4 }));
    for (int l = 0; l < numLayers; ++l)
        if (loadedSamples[(size_t) l] != nullptr)
            publishSample (l, nullptr);

    currentProgram = index;
    presetName = preset.name;
    stateVersion++;
}

bool LayerStackProcessor::saveUserPreset (const juce::String& name, const juce::String& category, juce::String& error)
{
    const auto cleanName = juce::File::createLegalFileName (name.trim());
    if (cleanName.isEmpty())
    {
        error = "Please give the preset a name.";
        return false;
    }

    const auto folder = userPresetFolder().getChildFile (juce::File::createLegalFileName (category));
    if (! folder.createDirectory())
    {
        error = "Couldn't create " + folder.getFullPathName();
        return false;
    }

    presetName = name.trim();
    juce::XmlElement root ("LayerStackPreset");
    root.setAttribute ("name", presetName);
    root.setAttribute ("category", category);
    if (auto stateXml = buildState().createXml())
        root.addChildElement (stateXml.release());

    const auto file = folder.getChildFile (cleanName + ".lspreset");
    if (! root.writeTo (file))
    {
        error = "Couldn't write " + file.getFullPathName();
        return false;
    }
    stateVersion++;
    return true;
}

bool LayerStackProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("LayerStackPreset"))
        return false;
    auto* stateXml = xml->getChildByName (apvts.state.getType());
    if (stateXml == nullptr)
        return false;

    restoreState (juce::ValueTree::fromXml (*stateXml));
    presetName = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
    stateVersion++;
    return true;
}

// ---------------------------------------------------------------- state

juce::ValueTree LayerStackProcessor::buildState()
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName (extraTag), nullptr);

    juce::ValueTree extra (extraTag);
    extra.setProperty ("arpMask", (int) arpStepMask.load(), nullptr);
    juce::StringArray velocities;
    for (auto& v : arpStepVelocity)
        velocities.add (juce::String (v.load(), 3));
    extra.setProperty ("arpVelocities", velocities.joinIntoString (","), nullptr);
    extra.setProperty ("gateMask", (int) gateStepMask.load(), nullptr);
    extra.setProperty ("fxOrder", (int) fxOrderPacked.load(), nullptr);
    extra.setProperty ("program", currentProgram, nullptr);
    extra.setProperty ("presetName", presetName, nullptr);
    for (int l = 0; l < numLayers; ++l)
        if (const auto& s = loadedSamples[(size_t) l])
            extra.setProperty ("sample" + juce::String (l + 1), juce::String (s->path), nullptr);
    state.appendChild (extra, nullptr);
    return state;
}

void LayerStackProcessor::restoreState (const juce::ValueTree& stateIn)
{
    auto state = stateIn.createCopy();
    const auto extra = state.getChildWithName (extraTag);
    state.removeChild (extra, nullptr);
    apvts.replaceState (state);

    if (extra.isValid())
    {
        arpStepMask.store ((juce::uint32) (int) extra.getProperty ("arpMask", 0xffff));
        juce::StringArray velocities;
        velocities.addTokens (extra.getProperty ("arpVelocities").toString(), ",", "");
        for (int i = 0; i < maxSteps; ++i)
            arpStepVelocity[(size_t) i].store (i < velocities.size() ? juce::jlimit (0.05f, 1.0f, velocities[i].getFloatValue()) : 1.0f);
        gateStepMask.store ((juce::uint32) (int) extra.getProperty ("gateMask", 0x5555));
        fxOrderPacked.store (packOrder (unpackOrder ((juce::uint32) (int) extra.getProperty ("fxOrder", 0))));
        currentProgram = extra.getProperty ("program", 0);
        presetName = extra.getProperty ("presetName", "Init").toString();

        for (int l = 0; l < numLayers; ++l)
        {
            const auto path = extra.getProperty ("sample" + juce::String (l + 1)).toString();
            juce::String error;
            if (path.isNotEmpty() && juce::File (path).existsAsFile())
                loadSample (l, juce::File (path), error);
            else
                publishSample (l, nullptr);
        }
    }
    stateVersion++;
}

void LayerStackProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = buildState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LayerStackProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            restoreState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* LayerStackProcessor::createEditor()
{
    return new LayerStackEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LayerStackProcessor();
}
