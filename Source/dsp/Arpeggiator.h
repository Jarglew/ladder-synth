#pragma once

#include "SynthParams.h"
#include <array>
#include <cstdint>
#include <vector>

namespace ladder
{

// Tempo-synced arpeggiator. It works in beats (quarter notes), not samples:
// Instrument owns the clock and asks it what is due.
//
// Input notes go in with noteOn / noteOff; the arpeggiator answers with note
// events through a sink, which Instrument forwards to the voice engine.
class Arpeggiator
{
public:
    // Receives the arpeggiator's output.
    struct Sink
    {
        virtual ~Sink() = default;
        virtual void arpNoteOn (int note, float velocity) = 0;
        virtual void arpNoteOff (int note) = 0;
    };

    Arpeggiator();

    // currentBeat lets a rate change keep the step grid lined up with the clock.
    void setSettings (const ArpSettings& s, double currentBeat);
    const ArpSettings& getSettings() const { return settings; }

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void setSustain (bool down);

    // Puts the next step on the grid at or after beat. Use after the clock jumps.
    void resync (double beat);

    // Stops every sounding note and forgets the held ones.
    void clear (Sink& sink);
    // Stops every sounding note but keeps the held ones.
    void silence (Sink& sink);

    // Beat at which the next step or note-off is due, or a huge value if nothing is.
    double nextEventBeat() const;
    // Performs everything due at or before beat, in time order (note-offs first on ties).
    void processUntil (double beat, Sink& sink);

    bool hasHeldNotes() const { return ! held.empty(); }
    bool isIdle() const { return held.empty() && sounding.empty(); }
    int heldCount() const { return (int) held.size(); }

    static constexpr double never = 1.0e300;

private:
    struct Held
    {
        int note;
        float velocity;
        bool physicallyDown;
    };

    struct Sounding
    {
        int note;
        double offBeat;
    };

    ArpSettings settings;
    std::vector<Held> held;            // in the order they were played
    std::vector<Held> scratch;         // sorting space, so the audio thread never allocates
    std::vector<Sounding> sounding;
    std::vector<int> pattern;          // notes for one cycle of the current mode
    std::vector<float> patternVelocity;
    bool patternDirty = true;
    bool sustainDown = false;
    bool waitingForFreshChord = false; // hold mode: next key press starts a new chord

    std::int64_t nextStep = 0;
    int position = 0;                  // where we are in the pattern
    int lastRandom = -1;
    std::uint32_t rng = 0x2545f491u;

    double stepBeats() const { return noteLengthBeats (settings.rate); }
    double stepStartBeat (std::int64_t step) const;
    void fireStep (std::int64_t step, Sink& sink);
    void rebuildPattern();
    void removeHeld (int note);
    void startNote (int note, float velocity, double offBeat, Sink& sink);
    std::uint32_t nextRandom();
};

} // namespace ladder
