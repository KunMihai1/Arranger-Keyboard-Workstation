#include "ArrangerDefaults.h"

namespace ArrangerDefaults
{
    std::vector<SectionWindow> defaultWindowsForBars (int totalBars)
    {
        SectionWindow variation;
        variation.id            = juce::Uuid().toString();
        variation.name          = DefaultSectionNames::variation;
        variation.type          = ArrangerSectionType::Variation;
        variation.startBar      = ArrangerStyleDefaults::firstBar;
        variation.lengthBars    = juce::jmax (ArrangerStyleDefaults::minimumLengthBars, totalBars);
        variation.afterComplete = ArrangerAfterComplete::Loop;
        return { variation };
    }
}
