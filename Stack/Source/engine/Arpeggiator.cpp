#include "Arpeggiator.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace stack
{

void Arpeggiator::setSettings (const ArpSettings& s)
{
    const bool latchReleased = settings.latch && ! s.latch;
    settings = s;
    settings.steps = std::clamp (settings.steps, 1, maxSteps);
    settings.octaves = std::clamp (settings.octaves, 1, 4);

    if (latchReleased)
        for (int i = numHeld - 1; i >= 0; --i)
            if (! physicallyDown[(std::size_t) held[(std::size_t) i].note])
                removeHeld (held[(std::size_t) i].note);
}

void Arpeggiator::noteOn (int note, float velocity)
{
    if (note < 0 || note > 127)
        return;

    // With latch on, a fresh press after letting go of every key starts a new chord.
    if (settings.latch && std::none_of (physicallyDown.begin(), physicallyDown.end(), [] (bool b) { return b; }))
        numHeld = 0;

    physicallyDown[(std::size_t) note] = true;

    if (numHeld == 0)
        sequencePos = 0;

    for (int i = 0; i < numHeld; ++i)
    {
        if (held[(std::size_t) i].note == note)
        {
            held[(std::size_t) i].velocity = velocity;
            return;
        }
    }

    if (numHeld < maxHeld)
        held[(std::size_t) numHeld++] = { note, velocity };
}

void Arpeggiator::noteOff (int note)
{
    if (note < 0 || note > 127)
        return;
    physicallyDown[(std::size_t) note] = false;
    if (! settings.latch)
        removeHeld (note);
}

void Arpeggiator::removeHeld (int note)
{
    for (int i = 0; i < numHeld; ++i)
    {
        if (held[(std::size_t) i].note == note)
        {
            for (int j = i; j < numHeld - 1; ++j)
                held[(std::size_t) j] = held[(std::size_t) j + 1];
            --numHeld;
            return;
        }
    }
}

void Arpeggiator::reset()
{
    numHeld = 0;
    numSounding = 0;
    physicallyDown.fill (false);
    sequencePos = 0;
}

double Arpeggiator::stepTime (std::int64_t step) const
{
    const double len = rateInBeats (settings.rate);
    // Swing pushes every second step later, up to a triplet feel.
    const double swingOffset = (step & 1) ? settings.swing * len / 3.0 : 0.0;
    return (double) step * len + swingOffset;
}

int Arpeggiator::toOffset (double ppq, double ppqStart, double beatsPerSample, int offsetBase, int numSamples) const
{
    const double samples = std::ceil ((ppq - ppqStart) / beatsPerSample - 1.0e-9);
    return offsetBase + (int) std::clamp (samples, 0.0, (double) std::max (0, numSamples - 1));
}

void Arpeggiator::releaseSounding (int offset, NoteEventList& out)
{
    for (int i = 0; i < numSounding; ++i)
        out.add ({ NoteEvent::Type::noteOff, offset, sounding[(std::size_t) i].note, 0.0f });
    numSounding = 0;
}

void Arpeggiator::generate (double ppqStart, double beatsPerSample, int numSamples, int offsetBase, NoteEventList& out)
{
    if (numSamples <= 0 || beatsPerSample <= 0.0)
        return;

    const double ppqEnd = ppqStart + numSamples * beatsPerSample;
    const double len = rateInBeats (settings.rate);

    // First step at or after the start of this range.
    std::int64_t step = (std::int64_t) std::floor (ppqStart / len) - 1;
    while (stepTime (step) < ppqStart)
        ++step;

    for (;;)
    {
        double nextOff = std::numeric_limits<double>::max();
        int offIndex = -1;
        for (int i = 0; i < numSounding; ++i)
        {
            if (sounding[(std::size_t) i].offPpq < nextOff)
            {
                nextOff = sounding[(std::size_t) i].offPpq;
                offIndex = i;
            }
        }

        const double nextStep = stepTime (step);
        if (std::min (nextOff, nextStep) >= ppqEnd)
            break;

        // Note-offs win ties, so a step can re-trigger the note it just released.
        if (offIndex >= 0 && nextOff <= nextStep)
        {
            out.add ({ NoteEvent::Type::noteOff, toOffset (nextOff, ppqStart, beatsPerSample, offsetBase, numSamples),
                       sounding[(std::size_t) offIndex].note, 0.0f });
            sounding[(std::size_t) offIndex] = sounding[(std::size_t) --numSounding];
            continue;
        }

        startStep (step, nextStep, ppqStart, beatsPerSample, offsetBase, numSamples, out);
        ++step;
    }
}

int Arpeggiator::buildSequence (std::array<int, maxSequence>& seq, std::array<float, maxSequence>& vel) const
{
    std::array<Held, maxHeld> notes = held;
    const int n = numHeld;
    if (settings.mode != ArpMode::asPlayed)
        std::sort (notes.begin(), notes.begin() + n, [] (const Held& a, const Held& b) { return a.note < b.note; });

    // Ascending run across the octave range.
    std::array<int, maxSequence> up {};
    std::array<float, maxSequence> upVel {};
    int count = 0;
    for (int o = 0; o < settings.octaves; ++o)
        for (int i = 0; i < n; ++i)
            if (notes[(std::size_t) i].note + 12 * o <= 127 && count < maxSequence / 2)
            {
                up[(std::size_t) count] = notes[(std::size_t) i].note + 12 * o;
                upVel[(std::size_t) count] = notes[(std::size_t) i].velocity;
                ++count;
            }

    int len = 0;
    auto push = [&] (int idx)
    {
        seq[(std::size_t) len] = up[(std::size_t) idx];
        vel[(std::size_t) len] = upVel[(std::size_t) idx];
        ++len;
    };

    switch (settings.mode)
    {
        case ArpMode::up:
        case ArpMode::random:
        case ArpMode::asPlayed:
        case ArpMode::chord:
            for (int i = 0; i < count; ++i) push (i);
            break;
        case ArpMode::down:
            for (int i = count - 1; i >= 0; --i) push (i);
            break;
        case ArpMode::upDown:
            for (int i = 0; i < count; ++i) push (i);
            for (int i = count - 2; i >= 1; --i) push (i);
            break;
        case ArpMode::downUp:
            for (int i = count - 1; i >= 0; --i) push (i);
            for (int i = 1; i < count - 1; ++i) push (i);
            break;
    }
    return len;
}

void Arpeggiator::startStep (std::int64_t step, double stepPpq, double ppqStart, double beatsPerSample, int offsetBase,
                             int numSamples, NoteEventList& out)
{
    const int offset = toOffset (stepPpq, ppqStart, beatsPerSample, offsetBase, numSamples);

    // Anything still ringing from the previous step ends here.
    releaseSounding (offset, out);

    if (numHeld == 0)
        return;

    const auto patternIndex = (std::size_t) (((step % settings.steps) + settings.steps) % settings.steps);
    if (! settings.stepOn[patternIndex])
        return;

    std::array<int, maxSequence> seq {};
    std::array<float, maxSequence> vel {};
    const int len = buildSequence (seq, vel);
    if (len == 0)
        return;

    const double offPpq = stepPpq + std::clamp (settings.gate, 0.05f, 1.0f) * (stepTime (step + 1) - stepPpq);
    const float stepVel = std::max (0.05f, settings.stepVelocity[patternIndex]);

    auto play = [&] (int note, float velocity)
    {
        if (note < 0 || note > 127 || numSounding >= (int) sounding.size())
            return;
        out.add ({ NoteEvent::Type::noteOn, offset, note, std::clamp (velocity * stepVel, 0.0f, 1.0f) });
        sounding[(std::size_t) numSounding++] = { note, offPpq };
    };

    if (settings.mode == ArpMode::chord)
    {
        const int octave = sequencePos % settings.octaves;
        for (int i = 0; i < numHeld; ++i)
            play (held[(std::size_t) i].note + 12 * octave, held[(std::size_t) i].velocity);
        ++sequencePos;
        return;
    }

    int index;
    if (settings.mode == ArpMode::random)
    {
        randomState ^= randomState << 13;
        randomState ^= randomState >> 17;
        randomState ^= randomState << 5;
        index = (int) (randomState % (std::uint32_t) len);
    }
    else
    {
        index = sequencePos % len;
        ++sequencePos;
    }

    play (seq[(std::size_t) index], vel[(std::size_t) index]);
}

} // namespace stack
