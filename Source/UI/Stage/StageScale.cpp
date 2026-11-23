#include "StageScale.h"
#include "StageMetrics.h"

namespace StageScale
{
    StageRegions regionsFor (juce::Rectangle<int> bounds)
    {
        StageRegions regions;
        regions.scale = juce::jmin (static_cast<float> (bounds.getWidth())  / static_cast<float> (StageMetrics::referenceWidth),
                                    static_cast<float> (bounds.getHeight()) / static_cast<float> (StageMetrics::referenceHeight));

        const auto scaled = [&regions] (int referenceLength)
        {
            return juce::roundToInt (static_cast<float> (referenceLength) * regions.scale);
        };

        // removeFromTop / removeFromBottom clamp to what is left, so the note area never goes negative.
        regions.panel    = bounds.removeFromTop (scaled (StageMetrics::panelHeight));
        regions.felt     = bounds.removeFromTop (scaled (StageMetrics::feltHeight));
        regions.slip     = bounds.removeFromBottom (scaled (StageMetrics::slipHeight));
        regions.keys     = bounds.removeFromBottom (scaled (StageMetrics::keysHeight));
        regions.noteArea = bounds;
        return regions;
    }
}
