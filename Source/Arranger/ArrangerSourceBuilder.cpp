#include "ArrangerSourceBuilder.h"
#include "ArrangerChannels.h"
#include "ArrangerEnums.h"
#include "ArrangerPatternBuilder.h"

namespace ArrangerSourceBuilder
{
    std::vector<SourceTrackFile> fromTrackEntries (const std::vector<TrackEntry>& tracks,
                                                   double referenceBpm)
    {
        if (referenceBpm <= 0.0) referenceBpm = 120.0;

        std::vector<SourceTrackFile> out;
        int melodicIndex = 0;

        for (const auto& te : tracks)
        {
            const bool isPerc = (te.type == TrackType::Percussion);
            int channel;
            if (isPerc)
            {
                channel = ArrangerChannels::kDrumChannel;
            }
            else
            {
                channel = ArrangerChannels::melodicChannelForIndex (melodicIndex++);
                if (channel < 0)
                    continue; // melodic budget exhausted; drop this track
            }

            // A melodic track named like a bass ("Bass", "Bassline", ...) becomes the Bass part, so it
            // transposes AND honours Bass Inversion (lowest fingered note drives it). Others are Acc.
            const bool isBass = (! isPerc) && te.getDisplayName().containsIgnoreCase ("bass");

            SourceTrackFile st;
            st.id        = te.getUniqueID();
            st.name      = te.getDisplayName();
            st.partType  = isPerc ? ArrangerPartType::Drum
                         : isBass ? ArrangerPartType::Bass
                                  : ArrangerPartType::Acc;
            st.nttType   = ArrangerEnums::nttDefaultForPart (st.partType);   // Phase 7a default
            st.channel   = channel;
            st.instrument = te.instrumentAssociated;
            st.volume    = te.volumeAssociated;
            st.events    = ArrangerPatternBuilder::buildBeatEvents (te.sequence, referenceBpm, channel);
            out.push_back (std::move (st));
        }

        return out;
    }
}
