#include "ArrangerSourceBuilder.h"
#include "ArrangerChannels.h"
#include "ArrangerEnums.h"
#include "ArrangerPatternBuilder.h"

namespace ArrangerSourceBuilder
{
    std::vector<SourceTrackFile> fromTrackEntries (const std::vector<TrackEntry>& tracks,
                                                   double referenceBpm)
    {
        if (referenceBpm <= double {}) referenceBpm = ArrangerStyleDefaults::tempoBpm;

        std::vector<SourceTrackFile> out;
        int melodicIndex {};

        for (const auto& trackEntry : tracks)
        {
            const bool isPerc = (trackEntry.type == TrackType::Percussion);
            int channel;
            if (isPerc)
            {
                channel = ArrangerChannels::kDrumChannel;
            }
            else
            {
                channel = ArrangerChannels::melodicChannelForIndex (melodicIndex++);
                if (channel == ArrangerChannels::kNoChannel)
                    continue; // melodic budget exhausted; drop this track
            }

            // A melodic track named like a bass ("Bass", "Bassline", ...) becomes the Bass part, so it
            // transposes AND honours Bass Inversion (lowest fingered note drives it). Others are Acc.
            const bool isBass = (! isPerc) && trackEntry.getDisplayName().containsIgnoreCase (TrackRoles::bassNameMarker);

            SourceTrackFile sourceTrack;
            sourceTrack.id        = trackEntry.getUniqueID();
            sourceTrack.name      = trackEntry.getDisplayName();
            sourceTrack.partType  = isPerc ? ArrangerPartType::Drum
                         : isBass ? ArrangerPartType::Bass
                                  : ArrangerPartType::Acc;
            sourceTrack.nttType   = ArrangerEnums::nttDefaultForPart (sourceTrack.partType);   // Phase 7a default
            sourceTrack.channel   = channel;
            sourceTrack.instrument = trackEntry.instrumentAssociated;
            sourceTrack.volume    = trackEntry.volumeAssociated;
            sourceTrack.events    = ArrangerPatternBuilder::buildBeatEvents (trackEntry.sequence, referenceBpm, channel);
            out.push_back (std::move (sourceTrack));
        }

        return out;
    }
}
