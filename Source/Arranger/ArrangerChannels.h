#pragma once

namespace ArrangerChannels
{
    constexpr int kDrumChannel          = 10;
    constexpr int kFirstMelodicChannel  = 2;    // the first channel an accompaniment track gets
    constexpr int kNoChannel            = -1;   // the melodic channels are used up

    /** Melodic accompaniment channel for a 0-based melodic-track index.
        Order: 2..9 then 11..13, skipping hands (1,16), drums (10), record (14,15).
        Returns -1 when more than 11 melodic tracks are requested. */
    int melodicChannelForIndex (int melodicIndex);
}
