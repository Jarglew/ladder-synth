#pragma once

#include "../../../Source/dsp/Envelope.h"
#include "../../../Source/dsp/LadderFilter.h"
#include "../../../Source/dsp/Oscillator.h"
#include "SampleData.h"
#include "StackParams.h"
#include "Svf.h"
#include <cmath>
#include <cstdint>

namespace stack
{

// One layer of one playing note: oscillator(s) or sample, filter, two envelopes.
// Produces stereo (the supersaw spreads its seven saws across the field).
class LayerVoice
{
public:
    static constexpr int supersawVoices = 7;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        ampEnv.prepare (sampleRate);
        filterEnv.prepare (sampleRate);
        ladderL.prepare (sampleRate);
        ladderR.prepare (sampleRate);
        reset();
    }

    void reset()
    {
        ampEnv.reset();
        filterEnv.reset();
        ladderL.reset();
        ladderR.reset();
        svfL.reset();
        svfR.reset();
        sample = nullptr;
    }

    void seed (std::uint32_t s) { noiseSeed = s | 1u; }

    void start (const LayerSettings& s, const SampleData* sampleToPlay, float newVelocity)
    {
        velocity = newVelocity;
        sample = (s.wave == LayerWave::sample) ? sampleToPlay : nullptr;
        samplePos = 0.0;

        if (! ampEnv.isActive())
        {
            ladderL.reset();
            ladderR.reset();
            svfL.reset();
            svfR.reset();
            // Spread the supersaw phases so it starts thick, not as one big click.
            for (int i = 0; i < supersawVoices; ++i)
                osc[i].reset (std::fmod (0.137 * i + 0.071 * (noiseSeed & 7u), 1.0));
            sinePhase = 0.0;
            coefCountdown = 0;
        }

        ampEnv.noteOn();
        filterEnv.noteOn();
    }

    void release()
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
    }

    void kill() { reset(); }

    bool isActive() const { return ampEnv.isActive(); }

    // Called once per block before processSample.
    void setEnvelopes (const LayerSettings& s)
    {
        ampEnv.setSettings ({ s.ampEnv.attack, s.ampEnv.decay, s.ampEnv.sustain, s.ampEnv.release });
        filterEnv.setSettings ({ s.filterEnv.attack, s.filterEnv.decay, s.filterEnv.sustain, s.filterEnv.release });
    }

    // notePitch: the note's current pitch in MIDI semitones, including glide, bend and vibrato.
    // cutoffModOctaves: global modulation (LFO) applied to the cutoff.
    void processSample (const LayerSettings& s, double notePitch, double keyPitch, float cutoffModOctaves,
                        float& outL, float& outR)
    {
        if (! ampEnv.isActive())
            return;

        const double pitch = notePitch + 12.0 * s.octave + s.semitone + s.fineCents / 100.0;
        float l = 0.0f, r = 0.0f;
        renderSource (s, pitch, l, r);

        const float fEnv = filterEnv.process();
        const float aEnv = ampEnv.process();

        if (s.filterType != FilterType::off)
        {
            if (--coefCountdown <= 0)
            {
                // Recalculating the filter every 8 samples is inaudible and saves a lot of tan() calls.
                coefCountdown = 8;
                const float octaves = std::log2 (std::fmax (s.cutoffHz, 10.0f))
                                    + s.filterEnvAmount * 6.0f * fEnv
                                    + s.keyTrack * (float) (keyPitch - 60.0) / 12.0f
                                    + cutoffModOctaves;
                const float hz = std::exp2 (octaves);
                if (s.filterType == FilterType::lp24)
                    ladderCoefs = ladder::LadderFilter::makeCoefficients (hz, s.resonance, sampleRate);
                else
                    svfCoefs = Svf::makeCoefficients (hz, s.resonance, sampleRate);
            }

            switch (s.filterType)
            {
                case FilterType::lp24:
                    l = ladderL.process (l, ladderCoefs, 1.0f);
                    r = ladderR.process (r, ladderCoefs, 1.0f);
                    break;
                case FilterType::lp12:
                    l = svfL.process (l, svfCoefs, Svf::Mode::lowPass);
                    r = svfR.process (r, svfCoefs, Svf::Mode::lowPass);
                    break;
                case FilterType::hp12:
                    l = svfL.process (l, svfCoefs, Svf::Mode::highPass);
                    r = svfR.process (r, svfCoefs, Svf::Mode::highPass);
                    break;
                case FilterType::bp12:
                    l = svfL.process (l, svfCoefs, Svf::Mode::bandPass);
                    r = svfR.process (r, svfCoefs, Svf::Mode::bandPass);
                    break;
                case FilterType::off:
                    break;
            }
        }

        const float gain = aEnv * s.level * (1.0f - s.velocity + s.velocity * velocity);
        // Equal-power pan
        const float angle = (s.pan + 1.0f) * 0.25f * 3.14159265f;
        outL += l * gain * std::cos (angle) * 1.4142f;
        outR += r * gain * std::sin (angle) * 1.4142f;
    }

private:
    double sampleRate = 44100.0;
    ladder::Oscillator osc[supersawVoices];
    double sinePhase = 0.0;
    std::uint32_t noiseSeed = 0x2545f491u;

    const SampleData* sample = nullptr;
    double samplePos = 0.0;

    ladder::Envelope ampEnv, filterEnv;
    ladder::LadderFilter ladderL, ladderR;
    Svf svfL, svfR;
    ladder::LadderFilter::Coefficients ladderCoefs;
    Svf::Coefficients svfCoefs;
    int coefCountdown = 0;
    float velocity = 1.0f;

    static double hz (double midiPitch) { return 440.0 * std::exp2 ((midiPitch - 69.0) / 12.0); }

    float whiteNoise()
    {
        noiseSeed ^= noiseSeed << 13;
        noiseSeed ^= noiseSeed >> 17;
        noiseSeed ^= noiseSeed << 5;
        return (float) noiseSeed / 2147483648.0f - 1.0f;
    }

    void renderSource (const LayerSettings& s, double pitch, float& l, float& r)
    {
        using ladder::Waveform;

        switch (s.wave)
        {
            case LayerWave::saw:
            case LayerWave::square:
            case LayerWave::triangle:
            {
                const auto w = s.wave == LayerWave::saw ? Waveform::saw
                             : s.wave == LayerWave::square ? Waveform::square : Waveform::triangle;
                l = r = 0.5f * osc[0].process (w, hz (pitch), sampleRate);
                break;
            }

            case LayerWave::sine:
            {
                l = r = 0.5f * (float) std::sin (2.0 * 3.14159265358979 * sinePhase);
                sinePhase += hz (pitch) / sampleRate;
                if (sinePhase >= 1.0)
                    sinePhase -= 1.0;
                break;
            }

            case LayerWave::supersaw:
            {
                // Seven saws spread symmetrically in pitch (up to about +-0.6 semitones)
                // and in the stereo field. The centre saw is a little louder.
                for (int i = 0; i < supersawVoices; ++i)
                {
                    const float pos = (float) i / (float) (supersawVoices - 1) * 2.0f - 1.0f; // -1..1
                    const double detuneSemis = 0.6 * s.detune * pos;
                    const float v = osc[i].process (Waveform::saw, hz (pitch + detuneSemis), sampleRate)
                                  * (i == supersawVoices / 2 ? 1.0f : 0.8f);
                    const float p = pos * s.width;
                    l += v * (1.0f - p) * 0.5f;
                    r += v * (1.0f + p) * 0.5f;
                }
                l *= 0.32f;
                r *= 0.32f;
                break;
            }

            case LayerWave::noise:
                l = 0.4f * whiteNoise();
                r = 0.4f * whiteNoise();
                break;

            case LayerWave::sample:
            {
                if (sample == nullptr || sample->length() < 2)
                    break;

                const size_t len = sample->length();
                if (samplePos >= (double) (len - 1))
                {
                    if (! s.sampleLoop)
                        break;
                    samplePos = std::fmod (samplePos, (double) (len - 1));
                }

                const size_t i0 = (size_t) samplePos;
                const float frac = (float) (samplePos - (double) i0);
                l = sample->left[i0] + frac * (sample->left[i0 + 1] - sample->left[i0]);
                r = sample->right[i0] + frac * (sample->right[i0 + 1] - sample->right[i0]);

                const double ratio = std::exp2 ((pitch - (double) s.sampleRoot) / 12.0) * sample->sampleRate / sampleRate;
                samplePos += ratio;
                break;
            }
        }
    }
};

} // namespace stack
