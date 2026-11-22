#include "ArrangerChannels.h"
#include <iterator>

namespace ArrangerChannels
{
    // Arranger tracks avoid 1 and 16 (the hands), 10 (drums) and 14/15 (the record player).
    static const int melodicChannels[] = { 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13 };
    int melodicChannelForIndex (int melodicIndex)
    {
        const int channelCount = (int) std::size (melodicChannels);
        if (melodicIndex < int {} || melodicIndex >= channelCount)
            return kNoChannel;
        return melodicChannels[melodicIndex];
    }
}
