#include "StageFonts.h"

namespace
{
    juce::Typeface::Ptr typefaceFrom (const char* fontData, int fontDataSize)
    {
        return juce::Typeface::createSystemTypefaceFor (fontData, static_cast<size_t> (fontDataSize));
    }

    juce::Font fontWith (const juce::Typeface::Ptr& typeface, float height)
    {
        return juce::Font (juce::FontOptions (typeface).withHeight (height));
    }
}

namespace StageFonts
{
    juce::Font silkscreen (float height, bool semiBold)
    {
        static const juce::Typeface::Ptr mediumFace   = typefaceFrom (BinaryData::BarlowSemiCondensedMedium_ttf,
                                                                      BinaryData::BarlowSemiCondensedMedium_ttfSize);
        static const juce::Typeface::Ptr semiBoldFace = typefaceFrom (BinaryData::BarlowSemiCondensedSemiBold_ttf,
                                                                      BinaryData::BarlowSemiCondensedSemiBold_ttfSize);
        return fontWith (semiBold ? semiBoldFace : mediumFace, height);
    }

    juce::Font lcd (float height)
    {
        static const juce::Typeface::Ptr lcdFace = typefaceFrom (BinaryData::ShareTechMonoRegular_ttf,
                                                                 BinaryData::ShareTechMonoRegular_ttfSize);
        return fontWith (lcdFace, height);
    }
}
