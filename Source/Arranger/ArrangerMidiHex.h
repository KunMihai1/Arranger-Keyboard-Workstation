#pragma once
#include <JuceHeader.h>

namespace ArrangerMidiHex
{
    constexpr int  hexDigitsPerByte = 2;
    constexpr char paddingDigit     = '0';

    inline juce::String toHex (const juce::MidiMessage& message)
    {
        const juce::uint8* data = message.getRawData();
        const int size = message.getRawDataSize();
        juce::String out;
        for (int byteIndex {}; byteIndex < size; ++byteIndex)
            out += juce::String::toHexString (data[byteIndex]).paddedLeft (paddingDigit, hexDigitsPerByte).toUpperCase();
        return out;
    }

    inline juce::MidiMessage fromHex (const juce::String& hex)
    {
        std::vector<juce::uint8> bytes;
        for (int digitIndex {}; digitIndex + hexDigitsPerByte <= hex.length(); digitIndex += hexDigitsPerByte)
            bytes.push_back ((juce::uint8) hex.substring (digitIndex, digitIndex + hexDigitsPerByte).getHexValue32());
        if (bytes.empty())
            return juce::MidiMessage();
        return juce::MidiMessage (bytes.data(), (int) bytes.size());
    }
}
