#pragma once

#include "SynthParams.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace ladder
{

namespace fx
{
constexpr double twoPi = 6.283185307179586;

// Circular buffer read with linear interpolation, delay in samples (fractional).
inline float readFractional (const std::vector<float>& buf, int writePos, double delaySamples)
{
    const int size = (int) buf.size();
    double readPos = (double) writePos - delaySamples;
    while (readPos < 0.0)
        readPos += size;
    const int i0 = (int) readPos;
    const int i1 = (i0 + 1) % size;
    const float frac = (float) (readPos - i0);
    return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * frac;
}
} // namespace fx

// Two modulated delay taps, a quarter cycle apart: turns the mono synth into a wide stereo image.
class Chorus
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        buffer.assign ((size_t) (0.05 * sr) + 4, 0.0f);
        reset();
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
        phase = 0.0;
    }

    // Mono in, stereo out. With the chorus off it just copies the input to both sides.
    void process (const ChorusSettings& s, const float* in, float* left, float* right, int numSamples)
    {
        if (! s.enabled)
        {
            std::copy (in, in + numSamples, left);
            std::copy (in, in + numSamples, right);
            return;
        }

        const double baseMs = 7.0;
        const double depthMs = 0.5 + 5.5 * s.depth;
        const double inc = std::clamp ((double) s.rateHz, 0.01, 20.0) / sampleRate;
        const float dryGain = 1.0f - 0.45f * s.mix;
        const float wetGain = 0.75f * s.mix;
        const int size = (int) buffer.size();

        for (int i = 0; i < numSamples; ++i)
        {
            buffer[(size_t) writePos] = in[i];

            const double modL = 0.5 + 0.5 * std::sin (fx::twoPi * phase);
            const double modR = 0.5 + 0.5 * std::sin (fx::twoPi * (phase + 0.25));
            const double dL = (baseMs + depthMs * modL) * 0.001 * sampleRate;
            const double dR = (baseMs + depthMs * modR) * 0.001 * sampleRate;

            left[i] = dryGain * in[i] + wetGain * fx::readFractional (buffer, writePos, dL);
            right[i] = dryGain * in[i] + wetGain * fx::readFractional (buffer, writePos, dR);

            phase += inc;
            if (phase >= 1.0)
                phase -= 1.0;
            if (++writePos >= size)
                writePos = 0;
        }
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> buffer;
    int writePos = 0;
    double phase = 0.0;
};

// Tempo-synced stereo delay with optional ping-pong and a darkening feedback loop.
class StereoDelay
{
public:
    static constexpr double maxSeconds = 6.0;

    void prepare (double sr)
    {
        sampleRate = sr;
        const size_t size = (size_t) (maxSeconds * sr) + 4;
        bufL.assign (size, 0.0f);
        bufR.assign (size, 0.0f);
        reset();
    }

    void reset()
    {
        std::fill (bufL.begin(), bufL.end(), 0.0f);
        std::fill (bufR.begin(), bufR.end(), 0.0f);
        writePos = 0;
        lowL = lowR = 0.0f;
        currentDelay = -1.0;
    }

    void process (const DelaySettings& s, double bpm, float* left, float* right, int numSamples)
    {
        if (! s.enabled)
            return;

        const double seconds = std::min (noteLengthBeats (s.time) * 60.0 / std::max (bpm, 20.0), maxSeconds - 0.01);
        const double target = std::max (1.0, seconds * sampleRate);
        if (currentDelay < 0.0)
            currentDelay = target;

        const float feedback = std::clamp (s.feedback, 0.0f, 0.9f);
        const float dampCutoff = 18000.0f * std::pow (0.06f, s.damping); // about 1 kHz at full damping
        const float dampCoef = 1.0f - (float) std::exp (-fx::twoPi * dampCutoff / sampleRate);
        const float mix = s.mix;
        const int size = (int) bufL.size();

        for (int i = 0; i < numSamples; ++i)
        {
            currentDelay += (target - currentDelay) * 0.0005; // glides on tempo or time changes

            const float tapL = fx::readFractional (bufL, writePos, currentDelay);
            const float tapR = fx::readFractional (bufR, writePos, currentDelay);
            lowL += (tapL - lowL) * dampCoef;
            lowR += (tapR - lowR) * dampCoef;

            // Repeats pass through a soft saturator, so long feedback on a loud patch
            // thickens like tape instead of piling up without limit.
            const float fbL = feedback * saturate (lowL);
            const float fbR = feedback * saturate (lowR);
            const float inL = left[i], inR = right[i];
            if (s.pingPong)
            {
                // The input enters on the left only; each repeat crosses to the other side.
                bufL[(size_t) writePos] = 0.5f * (inL + inR) + fbR;
                bufR[(size_t) writePos] = fbL;
            }
            else
            {
                bufL[(size_t) writePos] = inL + fbL;
                bufR[(size_t) writePos] = inR + fbR;
            }

            left[i] = inL + mix * tapL;
            right[i] = inR + mix * tapR;

            if (++writePos >= size)
                writePos = 0;
        }
    }

private:
    // Unity gain for small signals, never above 2.
    static float saturate (float x) { return x / std::sqrt (1.0f + 0.25f * x * x); }

    double sampleRate = 44100.0;
    std::vector<float> bufL, bufR;
    int writePos = 0;
    float lowL = 0.0f, lowR = 0.0f;
    double currentDelay = -1.0;
};

// Schroeder-Moorer style reverb: eight damped comb filters into four allpasses per side,
// the right side slightly detuned from the left for width.
class Reverb
{
public:
    void prepare (double sr)
    {
        static constexpr int combTunings[numCombs] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static constexpr int allpassTunings[numAllpasses] = { 556, 441, 341, 225 };
        constexpr int stereoSpread = 23;
        const double scale = sr / 44100.0;

        for (size_t i = 0; i < numCombs; ++i)
        {
            combL[i].setSize ((int) (combTunings[i] * scale));
            combR[i].setSize ((int) ((combTunings[i] + stereoSpread) * scale));
        }
        for (size_t i = 0; i < numAllpasses; ++i)
        {
            allpassL[i].setSize ((int) (allpassTunings[i] * scale));
            allpassR[i].setSize ((int) ((allpassTunings[i] + stereoSpread) * scale));
        }
        reset();
    }

    void reset()
    {
        for (auto& c : combL) c.clear();
        for (auto& c : combR) c.clear();
        for (auto& a : allpassL) a.clear();
        for (auto& a : allpassR) a.clear();
    }

    void process (const ReverbSettings& s, float* left, float* right, int numSamples)
    {
        if (! s.enabled)
            return;

        const float roomFeedback = 0.7f + 0.28f * std::clamp (s.size, 0.0f, 1.0f);
        const float damp = 0.4f * std::clamp (s.damping, 0.0f, 1.0f);
        // Bigger rooms ring longer and build up more energy; scale the wet level to match.
        const float wet = 3.2f * s.mix * std::sqrt (1.0f - roomFeedback * roomFeedback);
        const float wet1 = wet * (0.5f + 0.5f * s.width);
        const float wet2 = wet * (0.5f - 0.5f * s.width);
        constexpr float inputGain = 0.015f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float input = (left[i] + right[i]) * inputGain;
            float outL = 0.0f, outR = 0.0f;

            for (size_t c = 0; c < numCombs; ++c)
            {
                outL += combL[c].process (input, roomFeedback, damp);
                outR += combR[c].process (input, roomFeedback, damp);
            }
            for (size_t a = 0; a < numAllpasses; ++a)
            {
                outL = allpassL[a].process (outL);
                outR = allpassR[a].process (outR);
            }

            left[i] += outL * wet1 + outR * wet2;
            right[i] += outR * wet1 + outL * wet2;
        }
    }

private:
    static constexpr size_t numCombs = 8;
    static constexpr size_t numAllpasses = 4;

    struct Comb
    {
        std::vector<float> buf;
        int pos = 0;
        float store = 0.0f;

        void setSize (int n) { buf.assign ((size_t) std::max (1, n), 0.0f); pos = 0; }
        void clear() { std::fill (buf.begin(), buf.end(), 0.0f); store = 0.0f; pos = 0; }

        float process (float in, float feedback, float damp)
        {
            const float out = buf[(size_t) pos];
            store = out * (1.0f - damp) + store * damp;
            buf[(size_t) pos] = in + store * feedback;
            if (++pos >= (int) buf.size())
                pos = 0;
            return out;
        }
    };

    struct Allpass
    {
        std::vector<float> buf;
        int pos = 0;

        void setSize (int n) { buf.assign ((size_t) std::max (1, n), 0.0f); pos = 0; }
        void clear() { std::fill (buf.begin(), buf.end(), 0.0f); pos = 0; }

        float process (float in)
        {
            const float delayed = buf[(size_t) pos];
            buf[(size_t) pos] = in + delayed * 0.5f;
            if (++pos >= (int) buf.size())
                pos = 0;
            return delayed - in;
        }
    };

    std::array<Comb, numCombs> combL, combR;
    std::array<Allpass, numAllpasses> allpassL, allpassR;
};

} // namespace ladder
