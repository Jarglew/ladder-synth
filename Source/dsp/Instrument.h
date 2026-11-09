#pragma once

#include "Arpeggiator.h"
#include "Effects.h"
#include "SynthEngine.h"
#include "TranceGate.h"
#include <array>
#include <vector>

namespace ladder
{

// The whole instrument, minus the plugin wrapper:
//
//   keys -> [arpeggiator] -> voice engine -> trance gate -> chorus -> delay -> reverb -> stereo out
//
// It keeps a beat clock for the arpeggiator, gate and delay. While the host is
// playing the clock follows the host's position; otherwise it free-runs at the
// host tempo and restarts on the first key press, so patterns start on the note.
class Instrument : private Arpeggiator::Sink
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void setParams (const SynthParams& p);
    const SynthParams& getParams() const { return params; }

    // Call once per audio block, before rendering it.
    // playing: the host transport is running and ppq is its position in beats.
    void setTransport (double bpm, bool playing, double ppq);

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void setSustainPedal (bool down);
    void setPitchBend (float normalised) { engine.setPitchBend (normalised); }
    void setModWheel (float value) { engine.setModWheel (value); }
    void allNotesOff();
    void reset();

    // Renders stereo, overwriting numSamples values in left and right.
    void render (float* left, float* right, int numSamples);

    int activeVoiceCount() const { return engine.activeVoiceCount(); }
    double getBeat() const { return beat; }
    // The gate step under the clock, or -1 while the gate is off.
    int currentGateStep() const;
    const SynthEngine& getEngine() const { return engine; }

private:
    SynthParams params;
    SynthEngine engine;
    Arpeggiator arp;
    TranceGate gate;
    Chorus chorus;
    StereoDelay delay;
    Reverb reverb;

    double sampleRate = 44100.0;
    std::vector<float> mono;
    int chunkSize = 256;

    double bpm = 120.0;
    double beat = 0.0;
    bool hostPlaying = false;

    std::array<bool, 128> keyDown {};
    std::array<float, 128> keyVelocity {};
    int keysDown = 0;
    bool sustainDown = false;
    bool arpWasOn = false;
    bool chorusWasOn = false, delayWasOn = false, reverbWasOn = false;

    void arpNoteOn (int note, float velocity) override { engine.noteOn (note, velocity); }
    void arpNoteOff (int note) override { engine.noteOff (note); }

    double beatsPerSample() const { return bpm / 60.0 / sampleRate; }
    void restartFreeClock();
    void renderSegment (float* left, float* right, int numSamples);
};

} // namespace ladder
