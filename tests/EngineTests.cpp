// Headless checks for the synth engine. No JUCE, no audio device.
// Build and run with CMake: ctest --test-dir build --output-on-failure

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

    std::printf ("\n%s: %d failure(s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
