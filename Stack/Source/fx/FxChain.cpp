#include "FxChain.h"
#include <cmath>

namespace stack
{

void FxChain::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    const juce::dsp::ProcessSpec spec { sr, (juce::uint32) juce::jmax (1, maxBlockSize), 2 };
    chorus.prepare (spec);
    phaser.prepare (spec);
    reverb.prepare (spec);

    // Room for 2 seconds of delay plus some slack.
    const auto delaySize = (size_t) (sr * 2.0) + 4;
    delayL.assign (delaySize, 0.0f);
    delayR.assign (delaySize, 0.0f);
    reset();
}

void FxChain::reset()
{
    chorus.reset();
    phaser.reset();
    reverb.reset();
    std::fill (delayL.begin(), delayL.end(), 0.0f);
    std::fill (delayR.begin(), delayR.end(), 0.0f);
    delayWrite = 0;
    delayDampL = delayDampR = 0.0f;
    toneStateL = toneStateR = 0.0f;
    smoothedDelaySamples = -1.0f;
}

void FxChain::process (juce::AudioBuffer<float>& buffer, const FxSettings& s, double bpm)
{
    for (int slot : s.order)
    {
        switch ((FxType) slot)
        {
            case FxType::distortion: if (s.distOn) distortion (buffer, s); break;
            case FxType::chorus:     if (s.chorusOn) runChorus (buffer, s); break;
            case FxType::phaser:     if (s.phaserOn) runPhaser (buffer, s); break;
            case FxType::delay:      if (s.delayOn) delay (buffer, s, bpm); break;
            case FxType::reverb:     if (s.reverbOn) runReverb (buffer, s); break;
        }
    }
    limiter (buffer);
}

void FxChain::distortion (juce::AudioBuffer<float>& buffer, const FxSettings& s)
{
    const float drive = 1.0f + 24.0f * s.distDrive * s.distDrive;
    const float makeup = 1.0f / std::tanh (std::sqrt (drive));
    // One-pole low-pass after the shaper; tone 1 = fully open.
    const float cutoff = 800.0f * std::pow (20000.0f / 800.0f, s.distTone);
    const float a = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * cutoff / (float) sampleRate);

    for (int ch = 0; ch < juce::jmin (2, buffer.getNumChannels()); ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        float& state = ch == 0 ? toneStateL : toneStateR;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float wet = std::tanh (d[i] * drive) * makeup;
            state += a * (wet - state);
            d[i] = d[i] + s.distMix * (state - d[i]);
        }
    }
}

void FxChain::runChorus (juce::AudioBuffer<float>& buffer, const FxSettings& s)
{
    chorus.setRate (0.05f + 4.0f * s.chorusRate * s.chorusRate);
    chorus.setDepth (s.chorusDepth);
    chorus.setCentreDelay (8.0f);
    chorus.setFeedback (0.0f);
    chorus.setMix (s.chorusMix);
    juce::dsp::AudioBlock<float> block (buffer);
    chorus.process (juce::dsp::ProcessContextReplacing<float> (block));
}

void FxChain::runPhaser (juce::AudioBuffer<float>& buffer, const FxSettings& s)
{
    phaser.setRate (0.05f + 4.0f * s.phaserRate * s.phaserRate);
    phaser.setDepth (s.phaserDepth);
    phaser.setFeedback (juce::jlimit (-0.95f, 0.95f, s.phaserFeedback * 0.95f));
    phaser.setCentreFrequency (900.0f);
    phaser.setMix (s.phaserMix);
    juce::dsp::AudioBlock<float> block (buffer);
    phaser.process (juce::dsp::ProcessContextReplacing<float> (block));
}

void FxChain::delay (juce::AudioBuffer<float>& buffer, const FxSettings& s, double bpm)
{
    const int size = (int) delayL.size();
    const float target = (float) juce::jlimit (1.0, (double) size - 3.0, s.delayBeats * 60.0 / juce::jmax (1.0, bpm) * sampleRate);
    if (smoothedDelaySamples < 0.0f)
        smoothedDelaySamples = target;

    const bool stereo = buffer.getNumChannels() > 1;
    float* l = buffer.getWritePointer (0);
    float* r = stereo ? buffer.getWritePointer (1) : l;
    const float fb = juce::jlimit (0.0f, 0.95f, s.delayFeedback);
    const float damp = 0.35f; // gentle darkening of each repeat

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        // Glide the delay time on tempo changes instead of jumping (no zipper clicks).
        smoothedDelaySamples += (target - smoothedDelaySamples) * 0.0005f;

        float readPos = (float) delayWrite - smoothedDelaySamples;
        if (readPos < 0.0f)
            readPos += (float) size;
        const int i0 = (int) readPos;
        const int i1 = (i0 + 1) % size;
        const float frac = readPos - (float) i0;
        const float tapL = delayL[(size_t) i0] + frac * (delayL[(size_t) i1] - delayL[(size_t) i0]);
        const float tapR = delayR[(size_t) i0] + frac * (delayR[(size_t) i1] - delayR[(size_t) i0]);

        delayDampL += damp * (tapL - delayDampL);
        delayDampR += damp * (tapR - delayDampR);

        const float inL = l[i], inR = stereo ? r[i] : l[i];
        if (s.delayPingPong)
        {
            // The input enters on the left, and the repeats bounce between sides.
            delayL[(size_t) delayWrite] = 0.5f * (inL + inR) + fb * delayDampR;
            delayR[(size_t) delayWrite] = fb * delayDampL;
        }
        else
        {
            delayL[(size_t) delayWrite] = inL + fb * delayDampL;
            delayR[(size_t) delayWrite] = inR + fb * delayDampR;
        }
        delayWrite = (delayWrite + 1) % size;

        l[i] = inL + s.delayMix * tapL;
        if (stereo)
            r[i] = inR + s.delayMix * tapR;
    }
}

void FxChain::runReverb (juce::AudioBuffer<float>& buffer, const FxSettings& s)
{
    juce::dsp::Reverb::Parameters p;
    p.roomSize = s.reverbSize;
    p.damping = s.reverbDamping;
    p.width = s.reverbWidth;
    p.wetLevel = s.reverbMix;
    p.dryLevel = 1.0f - 0.5f * s.reverbMix;
    reverb.setParameters (p);
    juce::dsp::AudioBlock<float> block (buffer);
    reverb.process (juce::dsp::ProcessContextReplacing<float> (block));
}

void FxChain::limiter (juce::AudioBuffer<float>& buffer)
{
    // Transparent below 0.8; above that, a smooth knee that never exceeds 1.0.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float x = d[i];
            const float ax = std::fabs (x);
            if (ax > 0.8f)
                d[i] = std::copysign (0.8f + 0.2f * std::tanh ((ax - 0.8f) / 0.2f), x);
        }
    }
}

} // namespace stack
