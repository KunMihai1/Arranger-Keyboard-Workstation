#pragma once
#include <JuceHeader.h>
#include "ArrangerModel.h"

namespace ArrangerEnums
{
    /** Enum names as written to .style files: the text must never change. */
    namespace SectionTypeNames
    {
        inline const juce::String intro     = "Intro";
        inline const juce::String variation = "Variation";
        inline const juce::String fill      = "Fill";
        inline const juce::String breakName = "Break";
        inline const juce::String ending    = "Ending";
        inline const juce::String countIn   = "CountIn";
    }

    namespace AfterCompleteNames
    {
        inline const juce::String loop        = "Loop";
        inline const juce::String fallThrough = "FallThrough";
        inline const juce::String stop        = "Stop";
    }

    namespace PartTypeNames
    {
        inline const juce::String drum          = "Drum";
        inline const juce::String percussion    = "Perc";
        inline const juce::String bass          = "Bass";
        inline const juce::String accompaniment = "Acc";
    }

    namespace NttTypeNames
    {
        inline const juce::String noTranspose = "NoTranspose";
        inline const juce::String parallel    = "Parallel";
        inline const juce::String chord       = "Chord";
        inline const juce::String fixed       = "Fixed";
    }

    inline juce::String toString (ArrangerSectionType sectionType)
    {
        using namespace SectionTypeNames;
        switch (sectionType)
        {
            case ArrangerSectionType::Intro:     return intro;
            case ArrangerSectionType::Variation: return variation;
            case ArrangerSectionType::Fill:      return fill;
            case ArrangerSectionType::Break:     return breakName;
            case ArrangerSectionType::Ending:    return ending;
            case ArrangerSectionType::CountIn:   return countIn;
        }
        return variation;
    }

    inline ArrangerSectionType sectionTypeFromString (const juce::String& name)
    {
        using namespace SectionTypeNames;
        if (name == intro)     return ArrangerSectionType::Intro;
        if (name == fill)      return ArrangerSectionType::Fill;
        if (name == breakName) return ArrangerSectionType::Break;
        if (name == ending)    return ArrangerSectionType::Ending;
        if (name == countIn)   return ArrangerSectionType::CountIn;
        return ArrangerSectionType::Variation;
    }

    inline juce::String toString (ArrangerAfterComplete afterComplete)
    {
        using namespace AfterCompleteNames;
        switch (afterComplete)
        {
            case ArrangerAfterComplete::Loop:        return loop;
            case ArrangerAfterComplete::FallThrough: return fallThrough;
            case ArrangerAfterComplete::Stop:        return stop;
        }
        return loop;
    }

    inline ArrangerAfterComplete afterFromString (const juce::String& name)
    {
        using namespace AfterCompleteNames;
        if (name == fallThrough) return ArrangerAfterComplete::FallThrough;
        if (name == stop)        return ArrangerAfterComplete::Stop;
        return ArrangerAfterComplete::Loop;
    }

    inline juce::String toString (ArrangerPartType partType)
    {
        using namespace PartTypeNames;
        switch (partType)
        {
            case ArrangerPartType::Drum: return drum;
            case ArrangerPartType::Perc: return percussion;
            case ArrangerPartType::Bass: return bass;
            case ArrangerPartType::Acc:  return accompaniment;
        }
        return accompaniment;
    }

    inline ArrangerPartType partFromString (const juce::String& name)
    {
        using namespace PartTypeNames;
        if (name == drum)       return ArrangerPartType::Drum;
        if (name == percussion) return ArrangerPartType::Perc;
        if (name == bass)       return ArrangerPartType::Bass;
        return ArrangerPartType::Acc;
    }

    inline juce::String toString (NttType nttType)
    {
        using namespace NttTypeNames;
        switch (nttType)
        {
            case NttType::NoTranspose: return noTranspose;
            case NttType::Parallel:    return parallel;
            case NttType::Chord:       return chord;
            case NttType::Fixed:       return fixed;
        }
        return chord;
    }

    inline NttType nttFromString (const juce::String& name)
    {
        using namespace NttTypeNames;
        if (name == noTranspose) return NttType::NoTranspose;
        if (name == parallel)    return NttType::Parallel;
        if (name == fixed)       return NttType::Fixed;
        return NttType::Chord;
    }

    /** Migration / fresh-track default derived from the part role. */
    inline NttType nttDefaultForPart (ArrangerPartType partType)
    {
        switch (partType)
        {
            case ArrangerPartType::Drum:
            case ArrangerPartType::Perc: return NttType::NoTranspose;
            case ArrangerPartType::Bass: return NttType::Parallel;
            case ArrangerPartType::Acc:  return NttType::Chord;
        }
        return NttType::Chord;
    }
}
