#include "Arpeggiator.h"
#include <algorithm>
#include <cmath>

namespace ladder
{

Arpeggiator::Arpeggiator()
{
    held.reserve (128);
    scratch.reserve (128);
    sounding.reserve (128);
    pattern.reserve (128 * 4 * 2);
    patternVelocity.reserve (128 * 4 * 2);
}

void Arpeggiator::setSettings (const ArpSettings& s, double currentBeat)
{
    const bool rateChanged = s.rate != settings.rate;
    const bool shapeChanged = s.mode != settings.mode || s.octaves != settings.octaves;
    const bool holdTurnedOff = settings.hold && ! s.hold;
    settings = s;
    settings.octaves = std::clamp (settings.octaves, 1, 4);
    settings.gate = std::clamp (settings.gate, 0.05f, 1.0f);
    settings.swing = std::clamp (settings.swing, 0.0f, 1.0f);

    if (shapeChanged)
        patternDirty = true;
    if (rateChanged)
        resync (currentBeat);

    if (holdTurnedOff && ! sustainDown)
    {
        // Let go of latched notes whose keys are already up.
        held.erase (std::remove_if (held.begin(), held.end(), [] (const Held& h) { return ! h.physicallyDown; }),
                    held.end());
        patternDirty = true;
        waitingForFreshChord = false;
    }
}

void Arpeggiator::noteOn (int note, float velocity)
{
    if (note < 0 || note > 127)
        return;

    if (held.empty())
    {
        position = 0;
        lastRandom = -1;
    }

    if (waitingForFreshChord)
    {
        // Hold mode: a new chord after letting go of every key replaces the latched one.
        held.clear();
        position = 0;
        waitingForFreshChord = false;
    }

    removeHeld (note);
    held.push_back ({ note, velocity, true });
    patternDirty = true;
}

void Arpeggiator::noteOff (int note)
{
    for (auto& h : held)
        if (h.note == note)
            h.physicallyDown = false;

    if (! settings.hold && ! sustainDown)
        removeHeld (note);

    if (settings.hold && std::none_of (held.begin(), held.end(), [] (const Held& h) { return h.physicallyDown; }))
        waitingForFreshChord = true;
}

void Arpeggiator::setSustain (bool down)
{
    if (down == sustainDown)
        return;
    sustainDown = down;

    if (! down && ! settings.hold)
    {
        held.erase (std::remove_if (held.begin(), held.end(), [] (const Held& h) { return ! h.physicallyDown; }),
                    held.end());
        patternDirty = true;
    }
}

void Arpeggiator::resync (double beat)
{
    const double len = stepBeats();
    nextStep = (std::int64_t) std::ceil (beat / len - 1.0e-9);
}

void Arpeggiator::silence (Sink& sink)
{
    for (const auto& s : sounding)
        sink.arpNoteOff (s.note);
    sounding.clear();
}

void Arpeggiator::clear (Sink& sink)
{
    silence (sink);
    held.clear();
    patternDirty = true;
    waitingForFreshChord = false;
    position = 0;
}

double Arpeggiator::stepStartBeat (std::int64_t step) const
{
    const double len = stepBeats();
    const double swingOffset = (step & 1) != 0 ? settings.swing * 0.5 * len : 0.0;
    return (double) step * len + swingOffset;
}

double Arpeggiator::nextEventBeat() const
{
    double next = held.empty() ? never : stepStartBeat (nextStep);
    for (const auto& s : sounding)
        next = std::min (next, s.offBeat);
    return next;
}

void Arpeggiator::processUntil (double beat, Sink& sink)
{
    // Loop because a step can schedule a note-off that is itself already due.
    for (int guard = 0; guard < 4096; ++guard)
    {
        double earliestOff = never;
        for (const auto& s : sounding)
            earliestOff = std::min (earliestOff, s.offBeat);
        const double stepBeat = held.empty() ? never : stepStartBeat (nextStep);

        if (earliestOff <= beat && earliestOff <= stepBeat)
        {
            for (size_t i = 0; i < sounding.size();)
            {
                if (sounding[i].offBeat <= earliestOff)
                {
                    sink.arpNoteOff (sounding[i].note);
                    sounding.erase (sounding.begin() + (std::ptrdiff_t) i);
                }
                else
                {
                    ++i;
                }
            }
        }
        else if (stepBeat <= beat)
        {
            fireStep (nextStep, sink);
            ++nextStep;
        }
        else
        {
            break;
        }
    }
}

void Arpeggiator::fireStep (std::int64_t step, Sink& sink)
{
    if (patternDirty)
        rebuildPattern();
    if (pattern.empty())
        return;

    const double start = stepStartBeat (step);
    const double nextStart = stepStartBeat (step + 1);
    const double offBeat = std::min (start + settings.gate * stepBeats(), nextStart);

    if (settings.mode == ArpMode::chord)
    {
        // The whole chord on every step, climbing an octave per step.
        const int octave = position % settings.octaves;
        ++position;
        for (const auto& h : held)
        {
            const int n = h.note + 12 * octave;
            if (n <= 127)
                startNote (n, h.velocity, offBeat, sink);
        }
        return;
    }

    const int size = (int) pattern.size();
    int index = 0;
    if (settings.mode == ArpMode::random)
    {
        index = (int) (nextRandom() % (std::uint32_t) size);
        if (size > 1 && index == lastRandom)
            index = (index + 1 + (int) (nextRandom() % (std::uint32_t) (size - 1))) % size;
        lastRandom = index;
    }
    else
    {
        index = position % size;
        position = (position + 1) % size;
    }

    startNote (pattern[(size_t) index], patternVelocity[(size_t) index], offBeat, sink);
}

void Arpeggiator::startNote (int note, float velocity, double offBeat, Sink& sink)
{
    // Retrigger cleanly if this note is still sounding from an earlier step.
    for (size_t i = 0; i < sounding.size(); ++i)
    {
        if (sounding[i].note == note)
        {
            sink.arpNoteOff (note);
            sounding.erase (sounding.begin() + (std::ptrdiff_t) i);
            break;
        }
    }
    sink.arpNoteOn (note, velocity);
    sounding.push_back ({ note, offBeat });
}

void Arpeggiator::rebuildPattern()
{
    patternDirty = false;
    pattern.clear();
    patternVelocity.clear();
    if (held.empty())
        return;

    scratch.assign (held.begin(), held.end());
    if (settings.mode != ArpMode::asPlayed)
        std::sort (scratch.begin(), scratch.end(), [] (const Held& a, const Held& b) { return a.note < b.note; });

    for (int oct = 0; oct < settings.octaves; ++oct)
    {
        for (const auto& h : scratch)
        {
            const int n = h.note + 12 * oct;
            if (n <= 127)
            {
                pattern.push_back (n);
                patternVelocity.push_back (h.velocity);
            }
        }
    }

    if (settings.mode == ArpMode::down)
    {
        std::reverse (pattern.begin(), pattern.end());
        std::reverse (patternVelocity.begin(), patternVelocity.end());
    }
    else if (settings.mode == ArpMode::upDown && pattern.size() > 2)
    {
        // Up then back down, without repeating the top and bottom notes.
        for (int i = (int) pattern.size() - 2; i >= 1; --i)
        {
            pattern.push_back (pattern[(size_t) i]);
            patternVelocity.push_back (patternVelocity[(size_t) i]);
        }
    }

    if (position >= (int) pattern.size())
        position = 0;
}

void Arpeggiator::removeHeld (int note)
{
    const auto before = held.size();
    held.erase (std::remove_if (held.begin(), held.end(), [note] (const Held& h) { return h.note == note; }), held.end());
    if (held.size() != before)
        patternDirty = true;
}

std::uint32_t Arpeggiator::nextRandom()
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

} // namespace ladder
