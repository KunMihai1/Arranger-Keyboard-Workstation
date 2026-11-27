#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

/*  The audio callback's MIDI, split per channel without allocating (audio thread rule: no allocation, no locks).
    prepare() reserves every buffer once, off the audio thread; split() then only clears and refills them, which
    keeps their storage. One pass over the block instead of one per channel. Events without a channel (SysEx)
    reach no synth, as before. */
class MidiChannelSplitter
{
public:
    static constexpr int channelCount = 16;

    /** Reserves room for this many short (3-byte) events in each channel's buffer. Not on the audio thread. */
    void prepare (int eventsPerChannel);

    /** Clears every channel and routes each channel event of the block to its channel (1-16). */
    void split (const juce::MidiBuffer& block);

    /** Channel 1-16. */
    const juce::MidiBuffer& channel (int midiChannel) const;

    /** Longest channel message (status + two data bytes); longer events (SysEx) are skipped unread. */
    static constexpr int largestShortMessage = 3;

    /** Bytes one short event takes in a juce::MidiBuffer (sample position, size, up to 3 message bytes). */
    static size_t bytesPerShortEvent();

private:
    std::array<juce::MidiBuffer, channelCount> buffers;
};
