#include "PlaybackRange.h"

namespace
{
    using Field = int PlayBackSettings::*;
    const Field noteFields[] { &PlayBackSettings::startNote, &PlayBackSettings::endNote,
                               &PlayBackSettings::leftHandBound, &PlayBackSettings::rightHandBound };

    int movedOnPiano (int note, int shift)
    {
        if (note == PlaybackSettingsKeys::notSet)
            return note;
        return juce::jlimit (PlaybackRange::lowestPianoNote, PlaybackRange::highestPianoNote, note + shift);
    }
}

namespace PlaybackRange
{
    int octaveShift (int lowestKey, bool pcKeyboard)
    {
        return pcKeyboard ? lowestKey - middleC : int {};
    }

    PlayBackSettings shown (const PlayBackSettings& saved, int shift)
    {
        auto now = saved;
        for (const auto field : noteFields)
            now.*field = movedOnPiano (saved.*field, shift);
        return now;
    }

    PlayBackSettings savedAfterEdit (const PlayBackSettings& saved, const PlayBackSettings& edited, int shift)
    {
        const auto wasShown = shown (saved, shift);
        auto after = saved;
        after.VID = edited.VID;
        after.PID = edited.PID;
        for (const auto field : noteFields)
            if (edited.*field != wasShown.*field)
                after.*field = edited.*field == PlaybackSettingsKeys::notSet ? edited.*field : edited.*field - shift;
        return after;
    }
}
