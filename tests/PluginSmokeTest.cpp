// Loads the real plugin processor (not just the engine) and exercises it the way
// a host would: odd block sizes, several sample rates, MIDI, presets, random
// parameter sweeps, state save/restore, and the editor. Optionally writes a PNG
// of the editor so the panel can be checked by eye.
//
// Usage: PluginSmokeTest [path/to/snapshot.png]

#include "../Source/PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <cstdio>
#include <set>


namespace
{
int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
    if (! ok)
        ++failures;
}

struct RenderStats
{
    bool finite = true;
    float peak = 0.0f;
    double rms = 0.0;
};

RenderStats render (juce::AudioProcessor& p, double sampleRate, int blockSize, double seconds,
                    std::function<void (juce::MidiBuffer&, int64_t)> addMidi)
{
    RenderStats stats;
    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;
    const auto total = (int64_t) (seconds * sampleRate);
    double sum = 0.0;
    int64_t count = 0;

    for (int64_t pos = 0; pos < total; pos += blockSize)
    {
        midi.clear();
        addMidi (midi, pos);
        p.processBlock (buffer, midi);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* d = buffer.getReadPointer (ch);
            for (int i = 0; i < blockSize; ++i)
            {
                if (! std::isfinite (d[i]))
                    stats.finite = false;
                stats.peak = std::max (stats.peak, std::abs (d[i]));
                sum += (double) d[i] * d[i];
                ++count;
            }
        }
    }
    stats.rms = std::sqrt (sum / (double) std::max<int64_t> (1, count));
    return stats;
}

// A host transport that is playing at a fixed tempo, advancing with each block.
struct FakeHost : juce::AudioPlayHead
{
    double bpm = 140.0, sampleRate = 48000.0;
    int64_t samples = 0;

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm (bpm);
        info.setIsPlaying (true);
        info.setTimeInSamples (samples);
        info.setPpqPosition ((double) samples / sampleRate * bpm / 60.0);
        return info;
    }
};

void setParam (juce::AudioProcessor& p, const juce::String& id, float plainValue)
{
    for (auto* param : p.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (ranged->paramID == id)
                ranged->setValueNotifyingHost (ranged->convertTo0to1 (plainValue));
}

// Holds a C major chord for half a second, then releases it.
void chord (juce::MidiBuffer& m, int64_t pos, double sampleRate)
{
    const auto releaseAt = (int64_t) (0.5 * sampleRate);
    if (pos == 0)
        for (int n : { 48, 60, 64, 67 })
            m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
    if (pos <= releaseAt && releaseAt < pos + 1)
        for (int n : { 48, 60, 64, 67 })
            m.addEvent (juce::MidiMessage::noteOff (1, n), 0);
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    std::unique_ptr<juce::AudioProcessor> proc (createPluginFilter());
    auto& p = *proc;
    p.setPlayConfigDetails (0, 2, 48000.0, 512);

    check (p.getNumPrograms() >= 8, "has factory presets (" + juce::String (p.getNumPrograms()) + ")");
    check (p.acceptsMidi() && ! p.producesMidi(), "is a MIDI instrument");

    // Sample rates and block sizes a host might use, including blocks bigger than announced.
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        for (int block : { 1, 32, 512, 2048 })
        {
            p.prepareToPlay (sr, block == 2048 ? 512 : block);
            p.setCurrentProgram (0);
            const auto s = render (p, sr, block, 0.25, [sr] (juce::MidiBuffer& m, int64_t pos)
            {
                if (pos == 0)
                    m.addEvent (juce::MidiMessage::noteOn (1, 57, (juce::uint8) 110), 0);
            });
            check (s.finite && s.rms > 0.005 && s.peak < 2.0f,
                   juce::String ("plays at ") + juce::String (sr, 0) + " Hz, block " + juce::String (block));
        }
    }

    p.prepareToPlay (48000.0, 512);

    // Every factory preset makes sound and stays sane.
    for (int i = 0; i < p.getNumPrograms(); ++i)
    {
        p.setCurrentProgram (i);
        const auto s = render (p, 48000.0, 512, 1.0, [] (juce::MidiBuffer& m, int64_t pos) { chord (m, pos, 48000.0); });
        check (s.finite && s.rms > 0.003 && s.peak < 2.0f,
               "preset \"" + p.getProgramName (i) + "\" peak " + juce::String (s.peak, 2));
    }

    // Controllers: pitch bend, mod wheel, sustain, all-notes-off.
    {
        p.setCurrentProgram (3);
        const auto s = render (p, 48000.0, 256, 1.0, [] (juce::MidiBuffer& m, int64_t pos)
        {
            if (pos == 0)
            {
                m.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0);
                m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 90), 1);
                m.addEvent (juce::MidiMessage::noteOff (1, 60), 100);
            }
            if (pos == 12288)
            {
                m.addEvent (juce::MidiMessage::pitchWheel (1, 16383), 0);
                m.addEvent (juce::MidiMessage::controllerEvent (1, 1, 127), 10);
            }
            if (pos == 36864)
                m.addEvent (juce::MidiMessage::allNotesOff (1), 0);
        });
        check (s.finite && s.rms > 0.005, "pitch bend, mod wheel and sustain pedal");
    }

    // Random parameter sweeps: nothing blows up whatever the knobs say.
    {
        juce::Random rng (1234);
        bool ok = true;
        float worst = 0.0f;
        for (int round = 0; round < 60; ++round)
        {
            for (auto* param : p.getParameters())
                param->setValueNotifyingHost (rng.nextFloat());
            const auto s = render (p, 48000.0, 512, 0.2, [] (juce::MidiBuffer& m, int64_t pos)
            {
                if (pos == 0)
                {
                    m.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 127), 0);
                    m.addEvent (juce::MidiMessage::noteOn (1, 84, (juce::uint8) 127), 3);
                }
            });
            ok = ok && s.finite && s.peak < 6.0f;
            worst = std::max (worst, s.peak);
            p.processBlock (*std::make_unique<juce::AudioBuffer<float>> (2, 16), *std::make_unique<juce::MidiBuffer>());
        }
        check (ok, "60 random settings stay finite and bounded (worst peak " + juce::String (worst, 2) + ")");
    }

    // Arpeggiator, trance gate and every effect, following a playing host.
    {
        p.setCurrentProgram (0);
        for (const char* id : { "arpOn", "gateOn", "chorusOn", "delayOn", "reverbOn" })
            setParam (p, id, 1.0f);
        setParam (p, "voiceMode", 1.0f);

        FakeHost host;
        p.setPlayHead (&host);
        juce::AudioBuffer<float> buffer (2, 480);
        juce::MidiBuffer midi;
        bool finite = true;
        double sideDifference = 0.0, energy = 0.0;
        std::set<int> stepsSeen;

        for (int block = 0; block < 200; ++block) // two seconds
        {
            midi.clear();
            if (block == 0)
                for (int n : { 60, 64, 67 })
                    midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 10);
            p.processBlock (buffer, midi);
            host.samples += buffer.getNumSamples();
            stepsSeen.insert (static_cast<LadderSynthProcessor&> (p).gateStepPlaying.load());

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const float l = buffer.getSample (0, i), r = buffer.getSample (1, i);
                finite = finite && std::isfinite (l) && std::isfinite (r);
                sideDifference += std::abs (l - r);
                energy += std::abs (l) + std::abs (r);
            }
        }
        p.setPlayHead (nullptr);

        check (finite && energy > 100.0, "arp + gate + chorus + delay + reverb with a playing host");
        check (sideDifference > energy * 0.05, "the effects make a stereo image");
        check (stepsSeen.size() >= 16 && *stepsSeen.begin() >= 0, "gate step lights run through all 16 steps");

        for (const char* id : { "arpOn", "gateOn", "chorusOn", "delayOn", "reverbOn" })
            setParam (p, id, 0.0f);
    }

    // Save and restore.
    {
        p.setCurrentProgram (5);
        juce::MemoryBlock saved;
        p.getStateInformation (saved);
        std::vector<float> before;
        for (auto* param : p.getParameters())
            before.push_back (param->getValue());

        p.setCurrentProgram (0);
        p.setStateInformation (saved.getData(), (int) saved.getSize());

        bool same = true;
        size_t i = 0;
        for (auto* param : p.getParameters())
            same = same && std::abs (param->getValue() - before[i++]) < 1.0e-4f;
        check (same, "settings survive save and restore");
        check (p.getCurrentProgram() == 5, "restored preset number");
    }

    // Editor opens, resizes and can be drawn.
    {
        p.setCurrentProgram (3);
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditorAndMakeActive());
        check (editor != nullptr && editor->getWidth() > 0, "editor opens");
        if (editor != nullptr)
        {
            editor->setSize (editor->getWidth(), editor->getHeight());
            const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
            check (image.isValid(), "editor draws");

            editor->setSize (editor->getWidth() * 3 / 2, editor->getHeight() * 3 / 2);
            check (editor->createComponentSnapshot (editor->getLocalBounds()).isValid(), "editor draws at 150%");

            if (argc > 1)
            {
                juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
                out.deleteFile();
                juce::FileOutputStream stream (out);
                juce::PNGImageFormat png;
                check (stream.openedOk() && png.writeImageToStream (image, stream),
                       "wrote snapshot to " + out.getFullPathName());
            }
        }
    }

    p.releaseResources();
    proc.reset();
    std::printf ("\n%s: %d failure(s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
