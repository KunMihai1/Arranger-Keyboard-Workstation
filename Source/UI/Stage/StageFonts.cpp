#include "StageFonts.h"

namespace
{
    juce::Typeface::Ptr typefaceFrom (const char* fontData, int fontDataSize)
    {
        return juce::Typeface::createSystemTypefaceFor (fontData, static_cast<size_t> (fontDataSize));
    }

    juce::Font fontWith (const juce::Typeface::Ptr& typeface, float cssPixelSize, float letterSpacingEm)
    {
        juce::Font font (juce::FontOptions (typeface).withPointHeight (cssPixelSize));
        // JUCE spaces glyphs by kerning factor x font height (ascent + descent); CSS by em x font-size.
        font.setExtraKerningFactor (letterSpacingEm * font.getHeightToPointsFactor());
        return font;
    }
}

namespace StageFonts
{
    juce::Font silkscreen (float cssPixelSize, bool semiBold, float letterSpacingEm)
    {
        static const juce::Typeface::Ptr mediumFace   = typefaceFrom (BinaryData::BarlowSemiCondensedMedium_ttf,
                                                                      BinaryData::BarlowSemiCondensedMedium_ttfSize);
        static const juce::Typeface::Ptr semiBoldFace = typefaceFrom (BinaryData::BarlowSemiCondensedSemiBold_ttf,
                                                                      BinaryData::BarlowSemiCondensedSemiBold_ttfSize);
        return fontWith (semiBold ? semiBoldFace : mediumFace, cssPixelSize, letterSpacingEm);
    }

    juce::Font lcd (float cssPixelSize)
    {
        static const juce::Typeface::Ptr lcdFace = typefaceFrom (BinaryData::ShareTechMonoRegular_ttf,
                                                                 BinaryData::ShareTechMonoRegular_ttfSize);
        return fontWith (lcdFace, cssPixelSize, {});
    }
}
