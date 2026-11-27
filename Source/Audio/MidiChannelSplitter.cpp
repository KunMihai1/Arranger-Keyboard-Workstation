#include "MidiChannelSplitter.h"

namespace
{
    constexpr int firstMidiChannel     = 1;
}

size_t MidiChannelSplitter::bytesPerShortEvent()
{
    return sizeof (juce::int32) + sizeof (juce::uint16) + static_cast<size_t> (largestShortMessage);
}

void MidiChannelSplitter::prepare (int eventsPerChannel)
{
    for (auto& buffer : buffers)
    {
        buffer.clear();
        buffer.ensureSize (bytesPerShortEvent() * static_cast<size_t> (eventsPerChannel));
    }
}

void MidiChannelSplitter::split (const juce::MidiBuffer& block)
{
    for (auto& buffer : buffers)
        buffer.clear();   // keeps the storage

    for (const auto metadata : block)
    {
        if (metadata.numBytes > largestShortMessage)
            continue;   // SysEx: no channel, and copying a long message into a MidiMessage would allocate
        const auto message = metadata.getMessage();
        const auto midiChannel = message.getChannel();
        if (midiChannel >= firstMidiChannel && midiChannel <= channelCount)
            buffers[static_cast<size_t> (midiChannel - firstMidiChannel)].addEvent (message, metadata.samplePosition);
    }
}

const juce::MidiBuffer& MidiChannelSplitter::channel (int midiChannel) const
{
    jassert (midiChannel >= firstMidiChannel && midiChannel <= channelCount);
    return buffers[static_cast<size_t> (juce::jlimit (firstMidiChannel, channelCount, midiChannel) - firstMidiChannel)];
}
