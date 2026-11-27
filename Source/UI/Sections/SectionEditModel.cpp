#include "SectionEditModel.h"
#include "Arranger/ArrangerEnums.h"

namespace
{
    constexpr int  firstBar       = 1;
    constexpr int  minimumBars    = 1;
    constexpr int  pastLastBar    = 1;     // a window may end on the bar line after the last bar
    constexpr int  numberStep     = 1;
    constexpr bool includeSeparator = false;
    constexpr bool ignoreCase     = false;
    const juce::String nameSeparator = " ";
    const juce::String varText       = "Var";
    const juce::String barWord       = " bar";
    const juce::String barsWord      = " bars";

    bool isWindow (const std::vector<SectionWindow>& windows, int index)
    {
        return juce::isPositiveAndBelow (index, (int) windows.size());
    }
}

namespace SectionEditModel
{
    const std::vector<ArrangerSectionType>& editableTypes()
    {
        static const std::vector<ArrangerSectionType> types { ArrangerSectionType::Intro, ArrangerSectionType::Variation,
                                                              ArrangerSectionType::Fill, ArrangerSectionType::Break,
                                                              ArrangerSectionType::Ending };
        return types;
    }

    juce::String nextName (const std::vector<SectionWindow>& windows, ArrangerSectionType type)
    {
        // The editor's own rule for Add (stable per-type numbers, gaps fine): the engine targets type + name.
        int highestNumber {};
        for (const auto& existing : windows)
            if (existing.type == type)
                highestNumber = juce::jmax (highestNumber, existing.name.fromLastOccurrenceOf (nameSeparator, includeSeparator, ignoreCase).getIntValue());
        return ArrangerEnums::toString (type) + nameSeparator + juce::String (highestNumber + numberStep);
    }

    bool stepType (std::vector<SectionWindow>& windows, int index, int direction)
    {
        if (! isWindow (windows, index))
            return false;
        const auto& types = editableTypes();
        const int count = (int) types.size();
        auto& window = windows[(size_t) index];
        const auto found = std::find (types.begin(), types.end(), window.type);
        const int current = found == types.end() ? int {} : (int) std::distance (types.begin(), found);
        const auto newType = types[(size_t) (((current + direction) % count + count) % count)];
        if (newType == window.type)
            return false;
        window.type = newType;
        window.name = {};                                   // not counted as one of its new type's numbers
        window.name = nextName (windows, newType);
        return true;
    }

    bool stepStart (std::vector<SectionWindow>& windows, int index, int direction, int totalBars)
    {
        if (! isWindow (windows, index))
            return false;
        auto& window = windows[(size_t) index];
        const int newStart = juce::jlimit (firstBar, juce::jmax (firstBar, totalBars + pastLastBar - window.lengthBars), window.startBar + direction);
        if (newStart == window.startBar)
            return false;
        window.startBar = newStart;
        return true;
    }

    bool stepLength (std::vector<SectionWindow>& windows, int index, int direction, int totalBars)
    {
        if (! isWindow (windows, index))
            return false;
        auto& window = windows[(size_t) index];
        const int newLength = juce::jlimit (minimumBars, juce::jmax (minimumBars, totalBars + pastLastBar - window.startBar), window.lengthBars + direction);
        if (newLength == window.lengthBars)
            return false;
        window.lengthBars = newLength;
        return true;
    }

    bool setAfterComplete (std::vector<SectionWindow>& windows, int index, ArrangerAfterComplete afterComplete)
    {
        if (! isWindow (windows, index) || windows[(size_t) index].afterComplete == afterComplete)
            return false;
        windows[(size_t) index].afterComplete = afterComplete;
        return true;
    }

    juce::String typeText (ArrangerSectionType type)
    {
        return type == ArrangerSectionType::Variation ? varText : ArrangerEnums::toString (type);
    }

    juce::String lengthText (int lengthBars)
    {
        return juce::String (lengthBars) + (lengthBars == minimumBars ? barWord : barsWord);
    }
}
