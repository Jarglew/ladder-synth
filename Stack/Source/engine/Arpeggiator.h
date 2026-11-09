#pragma once

#include "StackParams.h"
#include <array>
#include <cstdint>

namespace stack
{

struct NoteEvent
{
    enum class Type { noteOn, noteOff };
    Type type = Type::noteOn;
    int offset = 0;       // sample position within the block
    int note = 60;
    float velocity = 1.0f;
};

// Fixed-capacity event list, so nothing allocates on the audio thread.
struct NoteEventList
{
    static constexpr int capacity = 512;
    std::array<NoteEvent, capacity> events;
    int size = 0;

    void clear() { size = 0; }
    void add (const NoteEvent& e)
    {
        if (size < capacity)
            events[(std::size_t) size++] = e;
    }
};

// Tempo-synced arpeggiator with a 16-step on/off + velocity pattern.
// Steps line up with the host's beat grid when Logic is playing, and with the
// first key press when it is stopped.
class Arpeggiator
{
public:
    void setSettings (const ArpSettings& s);
    void noteOn (int note, float velocity);
    void noteOff (int note);
    void reset();                // forget everything (no note-offs generated)
    bool hasNotes() const { return numHeld > 0; }

    // Adds the note events falling in [ppqStart, ppqStart + numSamples * beatsPerSample)
    // to out, with offsets starting at offsetBase.
    void generate (double ppqStart, double beatsPerSample, int numSamples, int offsetBase, NoteEventList& out);

    // Releases whatever the arpeggiator is currently sounding, at the given offset.
    void releaseSounding (int offset, NoteEventList& out);

private:
    static constexpr int maxHeld = 32;
    static constexpr int maxSequence = maxHeld * 4 * 2;

    ArpSettings settings;

    struct Held { int note; float velocity; };
    std::array<Held, maxHeld> held {};
    int numHeld = 0;
    std::array<bool, 128> physicallyDown {};

    struct Sounding { int note; double offPpq; };
    std::array<Sounding, maxHeld * 4> sounding {};
    int numSounding = 0;

    int sequencePos = 0;
    std::uint32_t randomState = 0x6d2b79f5u;

    void removeHeld (int note);
    void startStep (std::int64_t step, double stepPpq, double ppqStart, double beatsPerSample, int offsetBase,
                    int numSamples, NoteEventList& out);
    int buildSequence (std::array<int, maxSequence>& seq, std::array<float, maxSequence>& vel) const;
    double stepTime (std::int64_t step) const;
    int toOffset (double ppq, double ppqStart, double beatsPerSample, int offsetBase, int numSamples) const;
};

} // namespace stack
