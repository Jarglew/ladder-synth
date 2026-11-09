// Headless checks for the synth engine. No JUCE, no audio device.
// Build and run with CMake: ctest --test-dir build --output-on-failure

#include "../Source/dsp/Instrument.h"
#include "../Source/dsp/SynthEngine.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

using namespace ladder;

namespace
{
constexpr double sampleRate = 48000.0;
int failures = 0;

void check (bool condition, const std::string& what)
{
    std::printf ("%s  %s\n", condition ? "PASS" : "FAIL", what.c_str());
    if (! condition)
        ++failures;
}

std::vector<float> renderSeconds (SynthEngine& engine, double seconds)
{
    std::vector<float> out ((size_t) (seconds * sampleRate));
    const int block = 256;
    for (size_t pos = 0; pos < out.size(); pos += block)
        engine.render (out.data() + pos, (int) std::min<size_t> (block, out.size() - pos));
    return out;
}

double rms (const std::vector<float>& x, size_t from = 0)
{
    double sum = 0.0;
    for (size_t i = from; i < x.size(); ++i)
        sum += (double) x[i] * x[i];
    return std::sqrt (sum / (double) std::max<size_t> (1, x.size() - from));
}

bool allFinite (const std::vector<float>& x)
{
    for (float v : x)
        if (! std::isfinite (v))
            return false;
    return true;
}

float peak (const std::vector<float>& x)
{
    float p = 0.0f;
    for (float v : x)
        p = std::max (p, std::fabs (v));
    return p;
}

// Rough pitch estimate from upward zero crossings over the second half of the buffer.
double estimateHz (const std::vector<float>& x)
{
    const size_t start = x.size() / 2;
    std::vector<double> crossings;
    for (size_t i = start + 1; i < x.size(); ++i)
        if (x[i - 1] < 0.0f && x[i] >= 0.0f)
            crossings.push_back ((double) i - x[i] / (x[i] - x[i - 1]));
    if (crossings.size() < 3)
        return 0.0;
    return sampleRate * (double) (crossings.size() - 1) / (crossings.back() - crossings.front());
}

SynthParams sineLikeParams()
{
    // Triangle through a wide-open, resonance-free filter: clean enough to count crossings.
    SynthParams p;
    p.osc[0].wave = Waveform::triangle;
    p.cutoffHz = 20000.0f;
    p.emphasis = 0.0f;
    p.contour = 0.0f;
    p.keyTrack = KeyTrack::off;
    p.ampEnv = { 0.001f, 0.1f, 1.0f, 0.05f };
    return p;
}

SynthEngine makeEngine (const SynthParams& p)
{
    SynthEngine e;
    e.prepare (sampleRate);
    e.setParams (p);
    return e;
}

// Collects what the arpeggiator plays, with the beat it happened on.
struct ArpLog : Arpeggiator::Sink
{
    struct Event { bool on; int note; double beat; };
    std::vector<Event> events;
    double now = 0.0;
    void arpNoteOn (int note, float) override { events.push_back ({ true, note, now }); }
    void arpNoteOff (int note) override { events.push_back ({ false, note, now }); }

    std::vector<int> ons() const
    {
        std::vector<int> r;
        for (const auto& e : events)
            if (e.on)
                r.push_back (e.note);
        return r;
    }
};

// Steps the arpeggiator's clock forward in small increments, like Instrument does.
void runArp (Arpeggiator& arp, ArpLog& log, double fromBeat, double toBeat)
{
    for (double b = fromBeat; b <= toBeat + 1.0e-9; b += 1.0 / 96.0)
    {
        log.now = b;
        arp.processUntil (b, log);
    }
}

std::vector<int> repeatPattern (std::vector<int> cycle, size_t count)
{
    std::vector<int> r;
    for (size_t i = 0; i < count; ++i)
        r.push_back (cycle[i % cycle.size()]);
    return r;
}

std::string join (const std::vector<int>& v)
{
    std::string s;
    for (int n : v)
        s += std::to_string (n) + " ";
    return s;
}

double windowRms (const std::vector<float>& x, size_t from, size_t to)
{
    double sum = 0.0;
    to = std::min (to, x.size());
    for (size_t i = from; i < to; ++i)
        sum += (double) x[i] * x[i];
    return std::sqrt (sum / (double) std::max<size_t> (1, to - from));
}

struct Stereo { std::vector<float> left, right; };

Stereo renderInstrument (Instrument& inst, double seconds, int block = 256)
{
    Stereo out;
    out.left.resize ((size_t) (seconds * sampleRate));
    out.right.resize (out.left.size());
    for (size_t pos = 0; pos < out.left.size(); pos += (size_t) block)
    {
        const int n = (int) std::min<size_t> ((size_t) block, out.left.size() - pos);
        inst.render (out.left.data() + pos, out.right.data() + pos, n);
    }
    return out;
}

SynthParams percussiveParams()
{
    // Quick attack and release, so each arpeggiated note stands apart.
    auto p = sineLikeParams();
    p.ampEnv = { 0.001f, 0.05f, 1.0f, 0.005f };
    return p;
}
} // namespace

int main()
{
    {
        auto e = makeEngine (SynthParams {});
        e.noteOn (48, 0.8f);
        const auto out = renderSeconds (e, 1.0);
        check (allFinite (out), "default patch renders finite samples");
        check (rms (out) > 0.01, "default patch is audible");
        check (peak (out) < 1.5f, "default patch stays in a sane range");
    }

    {
        auto e = makeEngine (sineLikeParams());
        e.noteOn (69, 1.0f);
        const double hz = estimateHz (renderSeconds (e, 0.5));
        check (std::fabs (hz - 440.0) < 2.0, "A4 plays at 440 Hz (got " + std::to_string (hz) + ")");
    }

    {
        auto p = sineLikeParams();
        p.osc[0].range = Range::ft16;
        auto e = makeEngine (p);
        e.noteOn (69, 1.0f);
        const double hz = estimateHz (renderSeconds (e, 0.5));
        check (std::fabs (hz - 220.0) < 1.5, "16' range drops an octave (got " + std::to_string (hz) + ")");
    }

    {
        auto p = sineLikeParams();
        p.bendRangeSemitones = 12.0f;
        auto e = makeEngine (p);
        e.noteOn (57, 1.0f);
        e.setPitchBend (1.0f);
        const double hz = estimateHz (renderSeconds (e, 0.5));
        check (std::fabs (hz - 440.0) < 2.0, "full bend up with 12-semitone range (got " + std::to_string (hz) + ")");
    }

    {
        auto bright = SynthParams {};
        bright.cutoffHz = 20000.0f;
        bright.contour = 0.0f;
        auto dark = bright;
        dark.cutoffHz = 150.0f;
        dark.keyTrack = KeyTrack::off;

        auto e1 = makeEngine (bright);
        e1.noteOn (72, 1.0f);
        auto e2 = makeEngine (dark);
        e2.noteOn (72, 1.0f);
        const double r1 = rms (renderSeconds (e1, 0.5), 4800);
        const double r2 = rms (renderSeconds (e2, 0.5), 4800);
        check (r2 < r1 * 0.25, "low cutoff strongly attenuates a C5 saw");
    }

    {
        auto p = SynthParams {};
        p.emphasis = 1.0f;
        p.drive = 1.0f;
        p.mixOsc[0] = p.mixOsc[1] = p.mixOsc[2] = 1.0f;
        p.mixNoise = 1.0f;
        p.osc[1].wave = Waveform::square;
        p.contour = 1.0f;
        auto e = makeEngine (p);
        e.noteOn (36, 1.0f);
        e.noteOn (60, 1.0f);
        const auto out = renderSeconds (e, 1.0);
        check (allFinite (out), "max emphasis + drive stays finite");
        check (peak (out) < 2.0f, "max emphasis + drive stays bounded");
    }

    {
        auto p = SynthParams {};
        p.mixOsc[0] = 0.0f;
        p.emphasis = 1.0f;
        p.cutoffHz = 1000.0f;
        p.contour = 0.0f;
        p.keyTrack = KeyTrack::off;
        p.mixNoise = 0.001f; // a whisper of noise to excite it
        auto e = makeEngine (p);
        e.noteOn (60, 1.0f);
        const auto out = renderSeconds (e, 1.0);
        check (rms (out, 24000) > 0.02, "full emphasis self-oscillates");
    }

    {
        auto p = sineLikeParams();
        auto e = makeEngine (p);
        e.noteOn (60, 1.0f);
        renderSeconds (e, 0.1);
        e.noteOff (60);
        renderSeconds (e, 0.5);
        check (e.activeVoiceCount() == 0, "voice frees itself after release");
    }

    {
        auto p = sineLikeParams();
        p.voiceMode = VoiceMode::mono;
        auto e = makeEngine (p);
        e.noteOn (69, 1.0f);
        e.noteOn (81, 1.0f);
        const double high = estimateHz (renderSeconds (e, 0.3));
        e.noteOff (81);
        const double back = estimateHz (renderSeconds (e, 0.3));
        check (std::fabs (high - 880.0) < 4.0, "mono: newest note sounds");
        check (std::fabs (back - 440.0) < 2.0, "mono: falls back to the held note on release");
        check (e.activeVoiceCount() == 1, "mono: only one voice used");
    }

    {
        auto p = sineLikeParams();
        p.voiceMode = VoiceMode::mono;
        p.glideSeconds = 0.5f;
        auto e = makeEngine (p);
        e.noteOn (57, 1.0f);
        renderSeconds (e, 0.3);
        e.noteOn (69, 1.0f);
        std::vector<float> firstBit ((size_t) (0.06 * sampleRate));
        e.render (firstBit.data(), (int) firstBit.size());
        const double during = estimateHz (firstBit);
        const double after = estimateHz (renderSeconds (e, 1.0));
        check (during > 225.0 && during < 400.0, "glide passes through in-between pitches (got " + std::to_string (during) + ")");
        check (std::fabs (after - 440.0) < 3.0, "glide arrives at the target (got " + std::to_string (after) + ")");
    }

    {
        auto p = SynthParams {};
        p.voiceMode = VoiceMode::poly;
        auto e = makeEngine (p);
        e.noteOn (60, 1.0f);
        e.noteOn (64, 1.0f);
        e.noteOn (67, 1.0f);
        renderSeconds (e, 0.1);
        check (e.activeVoiceCount() == 3, "poly: a three-note chord uses three voices");
        for (int n = 70; n < 80; ++n)
            e.noteOn (n, 1.0f);
        renderSeconds (e, 0.1);
        check (e.activeVoiceCount() == SynthEngine::maxVoices, "poly: voice stealing caps at the voice count");
    }

    {
        auto p = sineLikeParams();
        p.voiceMode = VoiceMode::poly;
        auto e = makeEngine (p);
        e.setSustainPedal (true);
        e.noteOn (60, 1.0f);
        renderSeconds (e, 0.05);
        e.noteOff (60);
        renderSeconds (e, 0.5);
        check (e.activeVoiceCount() == 1, "sustain pedal holds a released note");
        e.setSustainPedal (false);
        renderSeconds (e, 0.5);
        check (e.activeVoiceCount() == 0, "lifting the pedal releases it");
    }

    {
        const Waveform waves[] = { Waveform::triangle, Waveform::triSaw, Waveform::reverseSaw, Waveform::saw,
                                   Waveform::square, Waveform::widePulse, Waveform::narrowPulse };
        bool ok = true;
        for (auto w : waves)
        {
            auto p = SynthParams {};
            p.osc[0].wave = w;
            p.cutoffHz = 20000.0f;
            auto e = makeEngine (p);
            e.noteOn (60, 1.0f);
            const auto out = renderSeconds (e, 0.3);
            ok = ok && allFinite (out) && rms (out) > 0.01;
        }
        check (ok, "every waveform renders audibly");
    }

    {
        auto p = sineLikeParams();
        p.oscMod = true;
        p.modAmount = 1.0f;
        p.osc[2].range = Range::lo;
        p.osc[2].wave = Waveform::triangle;
        auto e = makeEngine (p);
        e.noteOn (69, 1.0f);
        const auto out = renderSeconds (e, 1.0);
        check (allFinite (out) && rms (out) > 0.05, "oscillator 3 as LFO modulates without breaking");
    }

    // ------------------------------------------------------------ arpeggiator

    {
        Arpeggiator arp;
        ArpLog log;
        ArpSettings s;
        s.enabled = true;
        s.mode = ArpMode::up;
        s.rate = NoteLength::sixteenth;
        s.gate = 0.5f;
        arp.setSettings (s, 0.0);
        arp.noteOn (67, 1.0f);
        arp.noteOn (60, 1.0f);
        arp.noteOn (64, 1.0f);
        arp.resync (0.0);
        runArp (arp, log, 0.0, 1.99);
        check (log.ons() == repeatPattern ({ 60, 64, 67 }, 8), "arp up plays the chord low to high: " + join (log.ons()));

        bool timing = true;
        int onIndex = 0;
        for (const auto& e : log.events)
        {
            if (e.on)
                timing = timing && std::fabs (e.beat - 0.25 * onIndex++) < 0.011;
        }
        check (timing, "arp sixteenths land every quarter beat");

        double firstOff = -1.0;
        for (const auto& e : log.events)
            if (! e.on && firstOff < 0.0)
                firstOff = e.beat;
        check (std::fabs (firstOff - 0.125) < 0.011, "arp gate 50% ends the note half way through the step");
    }

    {
        Arpeggiator arp;
        ArpLog log;
        ArpSettings s;
        s.enabled = true;
        s.mode = ArpMode::upDown;
        s.octaves = 2;
        arp.setSettings (s, 0.0);
        arp.noteOn (60, 1.0f);
        arp.noteOn (64, 1.0f);
        runArp (arp, log, 0.0, 2.99);
        check (log.ons() == repeatPattern ({ 60, 64, 72, 76, 72, 64 }, 12), "arp up/down over two octaves: " + join (log.ons()));
    }

    {
        Arpeggiator arp;
        ArpLog log;
        ArpSettings s;
        s.enabled = true;
        s.mode = ArpMode::down;
        arp.setSettings (s, 0.0);
        arp.noteOn (60, 1.0f);
        arp.noteOn (64, 1.0f);
        arp.noteOn (67, 1.0f);
        runArp (arp, log, 0.0, 1.49);
        check (log.ons() == repeatPattern ({ 67, 64, 60 }, 6), "arp down: " + join (log.ons()));
    }

    {
        Arpeggiator arp;
        ArpLog log;
        ArpSettings s;
        s.enabled = true;
        s.mode = ArpMode::asPlayed;
        arp.setSettings (s, 0.0);
        arp.noteOn (67, 1.0f);
        arp.noteOn (60, 1.0f);
        arp.noteOn (72, 1.0f);
        runArp (arp, log, 0.0, 1.49);
        check (log.ons() == repeatPattern ({ 67, 60, 72 }, 6), "arp as played keeps key order: " + join (log.ons()));
    }

    {
        Arpeggiator arp;
        ArpLog log;
        ArpSettings s;
        s.enabled = true;
        s.mode = ArpMode::random;
        arp.setSettings (s, 0.0);
        for (int n : { 60, 63, 67, 70 })
            arp.noteOn (n, 1.0f);
        runArp (arp, log, 0.0, 15.99);
        const auto ons = log.ons();
        bool inSet = ons.size() == 64, noRepeats = true;
        for (size_t i = 0; i < ons.size(); ++i)
        {
            inSet = inSet && (ons[i] == 60 || ons[i] == 63 || ons[i] == 67 || ons[i] == 70);
            if (i > 0)
                noRepeats = noRepeats && ons[i] != ons[i - 1];
        }
        check (inSet && noRepeats, "arp random stays in the chord and never repeats a note twice in a row");
    }

    {
        Arpeggiator arp;
        ArpLog log;
        ArpSettings s;
        s.enabled = true;
        s.mode = ArpMode::chord;
        s.octaves = 2;
        arp.setSettings (s, 0.0);
        arp.noteOn (60, 1.0f);
        arp.noteOn (64, 1.0f);
        runArp (arp, log, 0.0, 0.49);
        check (log.ons() == std::vector<int> ({ 60, 64, 72, 76 }), "arp chord mode plays the chord, then an octave up: " + join (log.ons()));
    }

    {
        Arpeggiator arp;
        ArpLog log;
        ArpSettings s;
        s.enabled = true;
        s.hold = true;
        arp.setSettings (s, 0.0);
        arp.noteOn (60, 1.0f);
        arp.noteOn (64, 1.0f);
        arp.noteOff (60);
        arp.noteOff (64);
        runArp (arp, log, 0.0, 0.99);
        check (log.ons() == repeatPattern ({ 60, 64 }, 4), "arp hold keeps playing after the keys are let go");
        arp.noteOn (70, 1.0f);
        log.events.clear();
        runArp (arp, log, 1.0, 1.99);
        check (log.ons() == repeatPattern ({ 70 }, 4), "arp hold: a new chord replaces the latched one");
    }

    {
        Arpeggiator arp;
        ArpLog log;
        ArpSettings s;
        s.enabled = true;
        arp.setSettings (s, 0.0);
        arp.noteOn (60, 1.0f);
        runArp (arp, log, 0.0, 0.3);
        arp.noteOff (60);
        log.events.clear();
        runArp (arp, log, 0.3, 2.0);
        check (log.ons().empty() && arp.isIdle(), "arp stops when the keys are let go (without hold)");
    }

    {
        Arpeggiator arp;
        ArpLog log;
        ArpSettings s;
        s.enabled = true;
        s.swing = 1.0f;
        s.rate = NoteLength::eighth;
        arp.setSettings (s, 0.0);
        arp.noteOn (60, 1.0f);
        runArp (arp, log, 0.0, 1.99);
        std::vector<double> onBeats;
        for (const auto& e : log.events)
            if (e.on)
                onBeats.push_back (e.beat);
        check (onBeats.size() == 4 && std::fabs (onBeats[1] - 0.75) < 0.011 && std::fabs (onBeats[2] - 1.0) < 0.011,
               "arp swing pushes every second step late");
    }

    // ------------------------------------------------------------ instrument

    {
        Instrument inst;
        inst.prepare (sampleRate, 256);
        auto p = percussiveParams();
        p.arp.enabled = true;
        p.arp.rate = NoteLength::sixteenth; // 6000 samples per step at 120 bpm, 48 kHz
        p.arp.gate = 0.5f;
        inst.setParams (p);
        inst.setTransport (120.0, false, 0.0);
        inst.noteOn (60, 1.0f);
        const auto out = renderInstrument (inst, 1.0, 100);

        bool pulses = true;
        for (size_t step = 0; step < 7; ++step)
        {
            const double on = windowRms (out.left, step * 6000 + 300, step * 6000 + 2800);
            const double off = windowRms (out.left, step * 6000 + 3600, step * 6000 + 5900);
            pulses = pulses && on > 0.05 && off < on * 0.05;
        }
        check (pulses, "instrument: a held key arpeggiates in sixteenths at 120 bpm");
    }

    {
        // Host playing from beat 0.1: the first arp note waits for the next sixteenth at 0.25.
        Instrument inst;
        inst.prepare (sampleRate, 512);
        auto p = percussiveParams();
        p.arp.enabled = true;
        inst.setParams (p);
        inst.setTransport (120.0, true, 0.1);
        inst.noteOn (60, 1.0f);
        const auto out = renderInstrument (inst, 0.25, 512);
        const size_t firstStep = (size_t) (0.15 * 24000.0); // 0.15 beats at 24000 samples per beat
        check (windowRms (out.left, 0, firstStep - 10) < 1.0e-6 && windowRms (out.left, firstStep + 50, firstStep + 2000) > 0.05,
               "instrument: arp locks to the host's beat grid");
    }

    {
        // Switching the arp off mid-phrase goes back to playing the held keys directly.
        Instrument inst;
        inst.prepare (sampleRate, 256);
        auto p = percussiveParams();
        p.arp.enabled = true;
        inst.setParams (p);
        inst.noteOn (69, 1.0f);
        renderInstrument (inst, 0.2);
        p.arp.enabled = false;
        inst.setParams (p);
        const auto out = renderInstrument (inst, 0.5);
        check (std::fabs (estimateHz (out.left) - 440.0) < 2.0 && windowRms (out.left, 12000, 24000) > 0.1,
               "instrument: turning the arp off keeps the held key sounding");
    }

    {
        Instrument inst;
        inst.prepare (sampleRate, 256);
        auto p = sineLikeParams();
        p.gate.enabled = true;
        p.gate.depth = 1.0f;
        p.gate.length = 1.0f;
        p.gate.smooth = 0.0f;
        for (int i = 0; i < GateSettings::numSteps; ++i)
            p.gate.steps[i] = (i % 2) == 0;
        inst.setParams (p);
        inst.setTransport (120.0, false, 0.0);
        inst.noteOn (69, 1.0f);
        const auto out = renderInstrument (inst, 1.0);
        bool chopped = true;
        for (size_t step = 2; step < 7; step += 2)
        {
            const double open = windowRms (out.left, step * 6000 + 300, step * 6000 + 5700);
            const double closed = windowRms (out.left, (step + 1) * 6000 + 300, (step + 1) * 6000 + 5700);
            chopped = chopped && open > 0.1 && closed < open * 0.01;
        }
        check (chopped, "trance gate chops a held note on the sixteenth grid");
    }

    {
        Instrument inst;
        inst.prepare (sampleRate, 256);
        auto p = sineLikeParams();
        p.gate.enabled = true;
        p.gate.depth = 0.0f;
        inst.setParams (p);
        inst.noteOn (69, 1.0f);
        const auto out = renderInstrument (inst, 0.5);
        const double a = windowRms (out.left, 6000, 12000), b = windowRms (out.left, 12000, 18000);
        check (std::fabs (a - b) < a * 0.02, "trance gate at zero depth leaves the level alone");
    }

    {
        StereoDelay d;
        d.prepare (sampleRate);
        DelaySettings s;
        s.enabled = true;
        s.time = NoteLength::quarter; // 0.5 s at 120 bpm
        s.feedback = 0.5f;
        s.damping = 0.0f;
        s.mix = 1.0f;
        s.pingPong = true;
        std::vector<float> l ((size_t) sampleRate * 2, 0.0f), r (l.size(), 0.0f);
        l[0] = r[0] = 1.0f;
        d.process (s, 120.0, l.data(), r.data(), (int) l.size());
        auto peakAround = [] (const std::vector<float>& x, size_t centre)
        {
            float pk = 0.0f;
            for (size_t i = centre - 50; i < centre + 50; ++i)
                pk = std::max (pk, std::fabs (x[i]));
            return pk;
        };
        check (peakAround (l, 24000) > 0.5f && peakAround (r, 24000) < 0.05f, "ping-pong delay: first echo on the left after a quarter note");
        check (peakAround (r, 48000) > 0.2f && peakAround (l, 48000) < 0.05f, "ping-pong delay: second echo crosses to the right");
    }

    {
        Reverb rv;
        rv.prepare (sampleRate);
        ReverbSettings s;
        s.enabled = true;
        s.mix = 1.0f;
        std::vector<float> l ((size_t) sampleRate * 6, 0.0f), r (l.size(), 0.0f);
        l[0] = r[0] = 1.0f;
        rv.process (s, l.data(), r.data(), (int) l.size());
        check (allFinite (l) && allFinite (r), "reverb stays finite");
        check (windowRms (l, 4800, 14400) > 0.001, "reverb leaves a tail after an impulse");
        check (windowRms (l, 5 * 48000, 6 * 48000) < windowRms (l, 4800, 14400) * 0.01, "reverb tail dies away");
    }

    {
        Instrument inst;
        inst.prepare (sampleRate, 256);
        auto p = sineLikeParams();
        p.chorus.enabled = true;
        p.chorus.mix = 1.0f;
        inst.setParams (p);
        inst.noteOn (60, 1.0f);
        const auto out = renderInstrument (inst, 0.5);
        double diff = 0.0;
        for (size_t i = 0; i < out.left.size(); ++i)
            diff += std::fabs (out.left[i] - out.right[i]);
        check (allFinite (out.left) && allFinite (out.right) && diff / (double) out.left.size() > 0.01,
               "chorus makes the left and right channels differ");
    }

    {
        // Everything on and pushed hard: still finite and bounded.
        Instrument inst;
        inst.prepare (sampleRate, 512);
        auto p = SynthParams {};
        p.voiceMode = VoiceMode::poly;
        p.emphasis = 0.9f;
        p.drive = 1.0f;
        p.arp.enabled = true;
        p.arp.mode = ArpMode::chord;
        p.arp.octaves = 4;
        p.arp.rate = NoteLength::thirtySecond;
        p.gate.enabled = true;
        p.chorus = { true, 5.0f, 1.0f, 1.0f };
        p.delay = { true, NoteLength::sixteenth, 0.9f, 0.0f, 1.0f, false };
        p.reverb = { true, 1.0f, 0.0f, 1.0f, 1.0f };
        inst.setParams (p);
        inst.setTransport (174.0, true, 0.0);
        for (int n : { 36, 48, 55, 60, 64, 67, 72 })
            inst.noteOn (n, 1.0f);
        const auto out = renderInstrument (inst, 3.0, 333);
        check (allFinite (out.left) && allFinite (out.right) && peak (out.left) < 8.0f && peak (out.right) < 8.0f,
               "arp + gate + full FX stays finite and bounded (peak " + std::to_string (peak (out.left)) + ")");
    }

    std::printf ("\n%s: %d failure(s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
