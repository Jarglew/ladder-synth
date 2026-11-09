// Headless checks for the Layer Stack engine. No JUCE, no audio device.

#include "../Source/engine/StackEngine.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace stack;
using std::size_t;

namespace
{
constexpr double sr = 48000.0;
int failures = 0;

void check (bool ok, const std::string& what)
{
    std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", what.c_str());
    if (! ok)
        ++failures;
}

struct Stereo
{
    std::vector<float> l, r;
};

// Renders in 256-sample blocks; events are given with absolute sample positions.
Stereo run (StackEngine& e, double seconds, std::vector<InputEvent> events = {}, Transport t = {})
{
    const int total = (int) (seconds * sr);
    Stereo out { std::vector<float> ((size_t) total), std::vector<float> ((size_t) total) };
    const int block = 256;
    for (int pos = 0; pos < total; pos += block)
    {
        const int len = std::min (block, total - pos);
        std::vector<InputEvent> blockEvents;
        for (auto ev : events)
            if (ev.offset >= pos && ev.offset < pos + len)
            {
                ev.offset -= pos;
                blockEvents.push_back (ev);
            }
        Transport bt = t;
        bt.ppqAtBlockStart = t.ppqAtBlockStart + pos * (t.bpm / 60.0 / sr);
        e.process (out.l.data() + pos, out.r.data() + pos, len, blockEvents.data(), (int) blockEvents.size(), bt);
    }
    return out;
}

InputEvent on (int note, int at, float vel = 1.0f) { return { InputEvent::Type::noteOn, at, note, vel }; }
InputEvent off (int note, int at) { return { InputEvent::Type::noteOff, at, note, 0.0f }; }

double rms (const std::vector<float>& x, size_t from = 0, size_t to = 0)
{
    if (to == 0 || to > x.size())
        to = x.size();
    double s = 0.0;
    for (size_t i = from; i < to; ++i)
        s += (double) x[i] * x[i];
    return std::sqrt (s / (double) std::max<size_t> (1, to - from));
}

bool finite (const Stereo& s)
{
    for (size_t i = 0; i < s.l.size(); ++i)
        if (! std::isfinite (s.l[i]) || ! std::isfinite (s.r[i]))
            return false;
    return true;
}

float peak (const Stereo& s)
{
    float p = 0.0f;
    for (size_t i = 0; i < s.l.size(); ++i)
        p = std::max ({ p, std::fabs (s.l[i]), std::fabs (s.r[i]) });
    return p;
}

double estimateHz (const std::vector<float>& x, size_t from, size_t to)
{
    std::vector<double> c;
    for (size_t i = from + 1; i < to; ++i)
        if (x[i - 1] < 0.0f && x[i] >= 0.0f)
            c.push_back ((double) i - x[i] / (x[i] - x[i - 1]));
    if (c.size() < 3)
        return 0.0;
    return sr * (double) (c.size() - 1) / (c.back() - c.front());
}

// Note onsets: positions where the signal rises out of near-silence.
std::vector<size_t> onsets (const std::vector<float>& x, float threshold = 0.01f)
{
    std::vector<size_t> result;
    bool quiet = true;
    size_t quietRun = 0;
    for (size_t i = 0; i < x.size(); ++i)
    {
        const bool loud = std::fabs (x[i]) > threshold;
        if (loud && quiet)
        {
            result.push_back (i);
            quiet = false;
            quietRun = 0;
        }
        else if (! loud)
        {
            if (++quietRun > 200)
                quiet = true;
        }
        else
            quietRun = 0;
    }
    return result;
}

StackParams sineParams()
{
    StackParams p;
    auto& l = p.layers[0];
    l.wave = LayerWave::sine;
    l.filterType = FilterType::off;
    l.ampEnv = { 0.001f, 0.05f, 1.0f, 0.01f };
    l.velocity = 0.0f;
    return p;
}

StackEngine make (const StackParams& p)
{
    StackEngine e;
    e.prepare (sr, 512);
    e.setParams (p);
    return e;
}

// For an arpeggio: which note played at each onset, by measuring pitch just after it.
std::vector<int> playedNotes (const std::vector<float>& x, const std::vector<size_t>& starts, int length)
{
    std::vector<int> notes;
    for (auto s : starts)
    {
        const double hz = estimateHz (x, s + 100, std::min (x.size(), s + (size_t) length));
        notes.push_back (hz > 0.0 ? (int) std::lround (69.0 + 12.0 * std::log2 (hz / 440.0)) : -1);
    }
    return notes;
}
} // namespace

int main()
{
    {
        auto e = make (StackParams {});
        const auto out = run (e, 1.0, { on (48, 0, 0.8f) });
        check (finite (out) && rms (out.l) > 0.01 && peak (out) < 1.5f, "default patch is audible and sane");
    }

    {
        auto e = make (sineParams());
        const auto out = run (e, 0.5, { on (69, 0) });
        const double hz = estimateHz (out.l, 4800, out.l.size());
        check (std::fabs (hz - 440.0) < 1.0, "sine layer plays A4 at 440 Hz (got " + std::to_string (hz) + ")");
    }

    {
        auto p = sineParams();
        p.layers[0].octave = -1;
        p.layers[0].semitone = 7;
        auto e = make (p);
        const auto out = run (e, 0.5, { on (69, 0) });
        const double hz = estimateHz (out.l, 4800, out.l.size());
        const double expected = 440.0 * std::pow (2.0, -5.0 / 12.0);
        check (std::fabs (hz - expected) < 1.0, "octave and semitone offsets (got " + std::to_string (hz) + ")");
    }

    {
        // Two layers stacked are louder than one.
        auto one = sineParams();
        auto two = sineParams();
        two.layers[1] = two.layers[0];
        two.layers[1].octave = 1;
        auto e1 = make (one);
        auto e2 = make (two);
        const double r1 = rms (run (e1, 0.5, { on (60, 0) }).l, 4800);
        const double r2 = rms (run (e2, 0.5, { on (60, 0) }).l, 4800);
        check (r2 > r1 * 1.25, "a second layer adds to the sound");
    }

    {
        // Pan hard left: the right channel is silent.
        auto p = sineParams();
        p.layers[0].pan = -1.0f;
        auto e = make (p);
        const auto out = run (e, 0.3, { on (60, 0) });
        check (rms (out.l) > 0.05 && rms (out.r) < 1.0e-4, "pan sends a layer to one side");
    }

    {
        // The supersaw is wide: left and right differ.
        StackParams p;
        p.layers[0].wave = LayerWave::supersaw;
        p.layers[0].width = 1.0f;
        p.layers[0].detune = 0.5f;
        auto e = make (p);
        const auto out = run (e, 0.5, { on (60, 0) });
        std::vector<float> diff (out.l.size());
        for (size_t i = 0; i < diff.size(); ++i)
            diff[i] = out.l[i] - out.r[i];
        check (finite (out) && rms (diff, 4800) > 0.02, "supersaw is spread in stereo");
    }

    {
        // Every filter type behaves: low-pass darkens, high-pass removes the low end.
        auto bright = StackParams {};
        bright.layers[0].filterType = FilterType::off;
        auto lp = bright;
        lp.layers[0].filterType = FilterType::lp24;
        lp.layers[0].cutoffHz = 200.0f;
        lp.layers[0].keyTrack = 0.0f;
        auto hp = bright;
        hp.layers[0].wave = LayerWave::sine;
        hp.layers[0].filterType = FilterType::hp12;
        hp.layers[0].cutoffHz = 4000.0f;
        hp.layers[0].keyTrack = 0.0f;
        auto hpRef = hp;
        hpRef.layers[0].filterType = FilterType::off;

        auto e0 = make (bright), e1 = make (lp), e2 = make (hp), e3 = make (hpRef);
        const double rBright = rms (run (e0, 0.4, { on (72, 0) }).l, 4800);
        const double rLp = rms (run (e1, 0.4, { on (72, 0) }).l, 4800);
        const double rHp = rms (run (e2, 0.4, { on (48, 0) }).l, 4800);
        const double rHpRef = rms (run (e3, 0.4, { on (48, 0) }).l, 4800);
        check (rLp < rBright * 0.3, "24 dB low-pass darkens a saw");
        check (rHp < rHpRef * 0.1, "high-pass removes a low sine");

        bool ok = true;
        for (auto type : { FilterType::lp12, FilterType::bp12 })
        {
            auto q = bright;
            q.layers[0].filterType = type;
            q.layers[0].resonance = 1.0f;
            auto e = make (q);
            const auto out = run (e, 0.3, { on (60, 0) });
            ok = ok && finite (out) && peak (out) < 3.0f;
        }
        check (ok, "12 dB filters at full resonance stay bounded");
    }

    {
        // Sample layer: a 220 Hz recording, root A3, played at A4, sounds at 440 Hz.
        SampleData sample;
        sample.sampleRate = 44100.0;
        for (int i = 0; i < 44100; ++i)
        {
            const float v = (float) std::sin (2.0 * 3.14159265358979 * 220.0 * i / 44100.0);
            sample.left.push_back (v);
            sample.right.push_back (v);
        }
        auto p = sineParams();
        p.layers[0].wave = LayerWave::sample;
        p.layers[0].sampleRoot = 57;
        auto e = make (p);
        e.setSample (0, &sample);
        const auto out = run (e, 0.3, { on (69, 0) });
        const double hz = estimateHz (out.l, 2400, out.l.size());
        check (std::fabs (hz - 440.0) < 1.5, "sample layer is repitched and rate-converted (got " + std::to_string (hz) + ")");

        // Without loop, the sample runs out; with loop, it keeps going.
        auto e2 = make (p);
        e2.setSample (0, &sample);
        const auto noLoop = run (e2, 1.2, { on (69, 0) });
        p.layers[0].sampleLoop = true;
        auto e3 = make (p);
        e3.setSample (0, &sample);
        const auto looped = run (e3, 1.2, { on (69, 0) });
        check (rms (noLoop.l, 48000) < 1.0e-3 && rms (looped.l, 48000) > 0.1, "sample loop on/off");
    }

    {
        auto p = sineParams();
        p.layers[0].wave = LayerWave::sample;   // no sample loaded
        auto e = make (p);
        const auto out = run (e, 0.2, { on (60, 0) });
        check (finite (out) && rms (out.l) == 0.0, "an empty sample layer is silent, not broken");
    }

    {
        auto e = make (StackParams {});
        std::vector<InputEvent> chord;
        for (int n = 0; n < 20; ++n)
            chord.push_back (on (40 + n, 0));
        run (e, 0.1, chord);
        check (e.activeVoiceCount() == StackEngine::maxVoices, "poly: 20 notes cap at 16 voices");
    }

    {
        auto p = sineParams();
        p.voiceMode = VoiceMode::mono;
        auto e = make (p);
        const auto out = run (e, 0.6, { on (69, 0), on (81, 9600), off (81, 19200) });
        check (std::fabs (estimateHz (out.l, 12000, 19000) - 880.0) < 3.0, "mono: newest note sounds");
        check (std::fabs (estimateHz (out.l, 21000, 28000) - 440.0) < 2.0, "mono: falls back to the held note");
        check (e.activeVoiceCount() == 1, "mono: one voice");
    }

    {
        // Arpeggiator, stopped transport, 120 bpm, 1/16 = 6000 samples per step.
        auto p = sineParams();
        p.arp.enabled = true;
        p.arp.mode = ArpMode::up;
        p.arp.rate = NoteRate::sixteenth;
        p.arp.gate = 0.5f;
        auto e = make (p);
        const auto out = run (e, 0.6, { on (64, 0), on (60, 10), on (67, 20) });
        const auto starts = onsets (out.l);
        check (starts.size() >= 4, "arp: plays a run of notes (" + std::to_string (starts.size()) + ")");
        if (starts.size() >= 4)
        {
            const double spacing = (double) (starts[3] - starts[0]) / 3.0;
            check (std::fabs (spacing - 6000.0) < 100.0, "arp: 1/16 steps at 120 bpm (" + std::to_string (spacing) + " samples)");
            const auto notes = playedNotes (out.l, starts, 2500);
            check (notes[0] == 60 && notes[1] == 64 && notes[2] == 67 && notes[3] == 60,
                   "arp up: C E G C (got " + std::to_string (notes[0]) + " " + std::to_string (notes[1]) + " "
                       + std::to_string (notes[2]) + " " + std::to_string (notes[3]) + ")");
        }
    }

    {
        auto p = sineParams();
        p.arp.enabled = true;
        p.arp.mode = ArpMode::down;
        p.arp.octaves = 2;
        p.arp.gate = 0.5f;
        auto e = make (p);
        const auto out = run (e, 0.6, { on (60, 0), on (64, 5) });
        const auto notes = playedNotes (out.l, onsets (out.l), 2500);
        check (notes.size() >= 4 && notes[0] == 76 && notes[1] == 72 && notes[2] == 64 && notes[3] == 60,
               "arp down over 2 octaves: E5 C5 E4 C4");
    }

    {
        // Pattern rests: step 2 off.
        auto p = sineParams();
        p.arp.enabled = true;
        p.arp.gate = 0.5f;
        p.arp.stepOn[1] = false;
        auto e = make (p);
        const auto out = run (e, 0.4, { on (60, 0) });
        const auto starts = onsets (out.l);
        check (starts.size() >= 2 && std::fabs ((double) (starts[1] - starts[0]) - 12000.0) < 100.0,
               "arp pattern: a switched-off step is a rest");
    }

    {
        // Synced to the host: playing from beat 0.1, the first step lands on beat 0.25.
        auto p = sineParams();
        p.arp.enabled = true;
        p.arp.gate = 0.5f;
        auto e = make (p);
        Transport t;
        t.playing = true;
        t.bpm = 120.0;
        t.ppqAtBlockStart = 0.1;
        const auto out = run (e, 0.3, { on (60, 0) }, t);
        const auto starts = onsets (out.l);
        const double expected = (0.25 - 0.1) / (120.0 / 60.0 / sr);
        check (! starts.empty() && std::fabs ((double) starts[0] - expected) < 60.0,
               "arp locks to the host beat grid (first step at " + std::to_string (starts.empty() ? 0 : starts[0]) + ")");
    }

    {
        // Latch: notes keep playing after release.
        auto p = sineParams();
        p.arp.enabled = true;
        p.arp.latch = true;
        p.arp.gate = 0.5f;
        auto e = make (p);
        const auto out = run (e, 1.0, { on (60, 0), off (60, 100) });
        check (rms (out.l, 36000) > 0.05, "arp latch keeps playing after the key is released");
    }

    {
        auto p = sineParams();
        p.arp.enabled = true;
        p.arp.gate = 0.5f;
        auto e = make (p);
        const auto out = run (e, 1.0, { on (60, 0), off (60, 100) });
        check (rms (out.l, 24000) < 1.0e-3, "without latch the arp stops when the key is released");
    }

    {
        // Trance gate: alternate steps duck the sound.
        auto p = sineParams();
        p.gate.enabled = true;
        p.gate.rate = NoteRate::sixteenth;
        p.gate.depth = 1.0f;
        auto e = make (p);
        const auto out = run (e, 0.5, { on (60, 0) });
        const double openStep = rms (out.l, 1000, 5500);    // step 0 (on)
        const double closedStep = rms (out.l, 7000, 11500); // step 1 (off)
        check (openStep > 0.1 && closedStep < openStep * 0.05, "trance gate chops on the 1/16 grid");
    }

    {
        // LFO to cutoff and to pitch, with every shape: stays finite.
        bool ok = true;
        for (auto shape : { LfoShape::sine, LfoShape::triangle, LfoShape::saw, LfoShape::square, LfoShape::sampleHold })
        {
            StackParams p;
            p.lfo1Shape = p.lfo2Shape = shape;
            p.lfo1ToCutoff = 1.0f;
            p.lfo2ToPitch = 1.0f;
            p.lfo1RateHz = 8.0f;
            auto e = make (p);
            const auto out = run (e, 0.3, { on (60, 0) });
            ok = ok && finite (out) && rms (out.l) > 0.01;
        }
        check (ok, "LFOs to cutoff and pitch, every shape");
    }

    {
        // Sustain pedal holds notes until it is lifted.
        auto p = sineParams();
        auto e = make (p);
        run (e, 0.2, { { InputEvent::Type::sustain, 0, 0, 1.0f }, on (60, 10), off (60, 2000) });
        check (e.activeVoiceCount() == 1, "sustain pedal holds a released note");
        run (e, 0.2, { { InputEvent::Type::sustain, 0, 0, 0.0f } });
        check (e.activeVoiceCount() == 0, "lifting the pedal releases it");
    }

    {
        // Everything at once, at extremes.
        StackParams p;
        for (int i = 0; i < numLayers; ++i)
        {
            auto& l = p.layers[(size_t) i];
            l.enabled = true;
            l.wave = (LayerWave) (i % 6);
            l.resonance = 1.0f;
            l.filterEnvAmount = 1.0f;
            l.level = 1.0f;
            l.filterType = (FilterType) (1 + i % 4);
        }
        p.arp.enabled = true;
        p.arp.mode = ArpMode::chord;
        p.arp.octaves = 4;
        p.gate.enabled = true;
        p.lfo1ToCutoff = 1.0f;
        p.volumeGain = 1.0f;
        auto e = make (p);
        const auto out = run (e, 1.0, { on (36, 0), on (48, 1), on (55, 2), on (60, 3), on (64, 4), on (72, 5) });
        check (finite (out) && peak (out) < 8.0f, "four layers + chord arp + gate at extremes stay finite (peak "
                                                   + std::to_string (peak (out)) + ")");
    }

    std::printf ("\n%s: %d failure(s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
