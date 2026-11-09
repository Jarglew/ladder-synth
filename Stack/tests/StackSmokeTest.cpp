// Drives the real Layer Stack processor and editor without a host: sample rates
// and block sizes, every preset, random parameter sweeps, patterns and effect
// order surviving save/restore, loading a WAV into a layer, user presets, and a
// PNG of every editor tab.
//
// Usage: StackSmokeTest [folder-for-snapshots]

#include "../Source/StackProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace
{
int failures = 0;

void check (bool ok, const juce::String& what)
{
    std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
    if (! ok)
        ++failures;
}

struct Stats
{
    bool finite = true;
    float peak = 0.0f;
    double rms = 0.0;
};

Stats render (juce::AudioProcessor& p, int block, double seconds, double sr,
              const std::function<void (juce::MidiBuffer&, juce::int64)>& midiAt)
{
    Stats s;
    juce::AudioBuffer<float> buffer (2, block);
    juce::MidiBuffer midi;
    const auto total = (juce::int64) (seconds * sr);
    double sum = 0.0;
    juce::int64 n = 0;
    for (juce::int64 pos = 0; pos < total; pos += block)
    {
        midi.clear();
        midiAt (midi, pos);
        p.processBlock (buffer, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < block; ++i)
            {
                const float v = buffer.getSample (ch, i);
                s.finite = s.finite && std::isfinite (v);
                s.peak = std::max (s.peak, std::abs (v));
                sum += (double) v * v;
                ++n;
            }
    }
    s.rms = std::sqrt (sum / (double) std::max<juce::int64> (1, n));
    return s;
}

auto chord (double sr)
{
    return [sr] (juce::MidiBuffer& m, juce::int64 pos)
    {
        if (pos == 0)
            for (int note : { 48, 60, 64, 67 })
                m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
        const auto releaseAt = (juce::int64) (0.8 * sr);
        if (pos <= releaseAt && releaseAt < pos + 512)
            for (int note : { 48, 60, 64, 67 })
                m.addEvent (juce::MidiMessage::noteOff (1, note), (int) (releaseAt - pos));
    };
}

void setParam (juce::AudioProcessor& p, const juce::String& id, float value)
{
    for (auto* param : p.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (r->getParameterID() == id)
                r->setValueNotifyingHost (r->convertTo0to1 (value));
}

void clickButton (juce::Component& root, const juce::String& text)
{
    for (auto* child : root.getChildren())
    {
        if (auto* b = dynamic_cast<juce::TextButton*> (child))
            if (b->getButtonText() == text)
            {
                if (b->onClick)
                    b->onClick();   // run the click handler right away (triggerClick is asynchronous)
                return;
            }
        clickButton (*child, text);
    }
}
} // namespace

int main (int argc, char** argv)
{
    // Keep user presets out of the real home folder.
    const auto home = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("layerstack-smoke-home");
    home.deleteRecursively();
    home.createDirectory();
    setenv ("HOME", home.getFullPathName().toRawUTF8(), 1);

    juce::ScopedJuceInitialiser_GUI gui;
    auto proc = std::make_unique<LayerStackProcessor>();
    auto& p = *proc;
    p.setPlayConfigDetails (0, 2, 48000.0, 512);

    check (p.getNumPrograms() >= 20, "has factory presets (" + juce::String (p.getNumPrograms()) + ")");

    for (double sr : { 44100.0, 48000.0, 96000.0 })
        for (int block : { 1, 64, 512, 2048 })
        {
            p.prepareToPlay (sr, block == 2048 ? 512 : block);
            p.loadFactoryPreset (1);
            const auto s = render (p, block, 0.25, sr, [] (juce::MidiBuffer& m, juce::int64 pos)
            {
                if (pos == 0)
                    m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 110), 0);
            });
            check (s.finite && s.rms > 0.005 && s.peak <= 1.0f,
                   "plays at " + juce::String (sr, 0) + " Hz, block " + juce::String (block));
        }

    p.prepareToPlay (48000.0, 512);

    for (int i = 0; i < p.getNumPrograms(); ++i)
    {
        p.setCurrentProgram (i);
        p.prepareToPlay (48000.0, 512);   // fresh effect tails per preset
        const auto s = render (p, 512, 2.5, 48000.0, chord (48000.0));
        check (s.finite && s.rms > 0.002 && s.peak <= 1.0f,
               "preset \"" + p.getProgramName (i) + "\" rms " + juce::String (s.rms, 3) + " peak " + juce::String (s.peak, 2));
    }

    {
        juce::Random rng (42);
        bool ok = true;
        for (int round = 0; round < 50; ++round)
        {
            for (auto* param : p.getParameters())
                param->setValueNotifyingHost (rng.nextFloat());
            const auto s = render (p, 512, 0.15, 48000.0, [] (juce::MidiBuffer& m, juce::int64 pos)
            {
                if (pos == 0)
                    for (int note : { 36, 60, 84 })
                        m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 127), 0);
            });
            ok = ok && s.finite && s.peak <= 1.0f;
        }
        check (ok, "50 random settings stay finite and under 0 dBFS");
    }

    {
        // Patterns, effect order and preset name survive save/restore.
        p.loadFactoryPreset (0);
        p.setArpStep (3, false, 0.5f);
        p.setArpStep (5, true, 0.3f);
        p.setGateStep (0, false);
        p.setGateStep (1, true);
        p.moveFx (0, 1);
        setParam (p, stack::layerId (2, stack::LayerParam::cutoff), 1234.0f);

        juce::MemoryBlock saved;
        p.getStateInformation (saved);
        p.loadFactoryPreset (5);
        p.setStateInformation (saved.getData(), (int) saved.getSize());

        const auto order = p.getFxOrder();
        const auto* cutoff = p.apvts.getRawParameterValue (stack::layerId (2, stack::LayerParam::cutoff));
        check (! p.getArpStepOn (3) && p.getArpStepOn (5) && std::abs (p.getArpStepVelocity (5) - 0.3f) < 0.01f,
               "arp pattern survives save and restore");
        check (! p.getGateStepOn (0) && p.getGateStepOn (1), "gate pattern survives save and restore");
        check (order[0] == 1 && order[1] == 0, "effect order survives save and restore");
        check (std::abs (cutoff->load() - 1234.0f) < 1.0f, "layer parameters survive save and restore");
    }

    {
        // Write a 220 Hz WAV and load it into layer 1.
        const auto wav = home.getChildFile ("tone.wav");
        {
            juce::AudioBuffer<float> tone (1, 44100);
            for (int i = 0; i < tone.getNumSamples(); ++i)
                tone.setSample (0, i, 0.8f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * i / 44100.0));
            juce::WavAudioFormat format;
            std::unique_ptr<juce::OutputStream> stream (wav.createOutputStream());
            auto writer = format.createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                              .withSampleRate (44100.0)
                                                              .withNumChannels (1)
                                                              .withBitsPerSample (24));
            if (writer != nullptr)
                writer->writeFromAudioSampleBuffer (tone, 0, tone.getNumSamples());
        }

        p.loadFactoryPreset (0);
        juce::String error;
        const bool loaded = p.loadSample (0, wav, error);
        check (loaded && p.getSampleName (0) == "tone", "loads a WAV into a layer" + (error.isEmpty() ? juce::String() : ": " + error));
        setParam (p, stack::layerId (0, stack::LayerParam::wave), (float) stack::LayerWave::sample);
        setParam (p, stack::layerId (0, stack::LayerParam::filterType), 0.0f);
        setParam (p, stack::layerId (0, stack::LayerParam::root), 57.0f);
        const auto s = render (p, 512, 0.5, 48000.0, [] (juce::MidiBuffer& m, juce::int64 pos)
        {
            if (pos == 0)
                m.addEvent (juce::MidiMessage::noteOn (1, 69, (juce::uint8) 120), 0);
        });
        check (s.finite && s.rms > 0.02, "sample layer plays the loaded file");

        juce::MemoryBlock saved;
        p.getStateInformation (saved);
        p.clearSample (0);
        p.setStateInformation (saved.getData(), (int) saved.getSize());
        check (p.getSampleName (0) == "tone", "sample reloads from its saved path");

        juce::String badError;
        check (! p.loadSample (1, home.getChildFile ("missing.wav"), badError) && badError.isNotEmpty(),
               "a missing file gives a readable error");
    }

    {
        p.loadFactoryPreset (2);
        juce::String error;
        check (p.saveUserPreset ("My Test Sound", "Lead", error), "saves a user preset" + (error.isEmpty() ? juce::String() : ": " + error));
        const auto presets = stack::scanUserPresets();
        check (presets.size() == 1 && presets[0].name == "My Test Sound", "finds the saved user preset");
        p.loadFactoryPreset (0);
        check (! presets.empty() && p.loadUserPreset (presets[0].file) && p.getPresetName() == "My Test Sound",
               "loads the user preset back");
    }

    {
        p.loadFactoryPreset (1);
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditorAndMakeActive());
        check (editor != nullptr, "editor opens");
        if (editor != nullptr)
        {
            juce::File folder;
            if (argc > 1)
            {
                folder = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
                folder.createDirectory();
            }

            bool allDrew = true;
            for (const auto* tab : { "LAYERS", "ARP", "GATE", "FX", "MOD", "BROWSE" })
            {
                clickButton (*editor, tab);
                const auto image = editor->createComponentSnapshot (editor->getLocalBounds());
                allDrew = allDrew && image.isValid();
                if (folder.isDirectory())
                {
                    auto file = folder.getChildFile (juce::String ("stack-") + juce::String (tab).toLowerCase() + ".png");
                    file.deleteFile();
                    juce::FileOutputStream out (file);
                    juce::PNGImageFormat().writeImageToStream (image, out);
                }
            }
            check (allDrew, "every tab draws");

            editor->setSize (editor->getWidth() * 3 / 2, editor->getHeight() * 3 / 2);
            check (editor->createComponentSnapshot (editor->getLocalBounds()).isValid(), "editor draws at 150%");
            clickButton (*editor, ">");
            check (p.getPresetName() != "Supersaw Anthem", "header arrow steps to the next preset");
        }
        editor.reset();
    }

    proc.reset();
    home.deleteRecursively();
    std::printf ("\n%s: %d failure(s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
