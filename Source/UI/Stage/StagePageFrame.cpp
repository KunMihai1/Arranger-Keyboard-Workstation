#include "StagePageFrame.h"
#include "StageFonts.h"
#include "../../Common/AppColours.h"

namespace
{
    constexpr bool  semiBold = true;
    constexpr float halves   = 2.0f;
}

StagePageFrame::StagePageFrame (const juce::String& pageTitle)
    : title (pageTitle)
{
    setOpaque (true);
}

void StagePageFrame::setScale (float newScale)
{
    scale = newScale;
    resized();
    repaint();
}

juce::String StagePageFrame::getTitle() const                 { return title; }
float StagePageFrame::scaled (int referenceLength) const      { return static_cast<float> (referenceLength) * scale; }

float StagePageFrame::titleHeight() const
{
    return StageFonts::silkscreen (StageMetrics::pageTitleSize * scale, semiBold, StageMetrics::pageTitleSpacing).getHeight();
}

juce::Rectangle<int> StagePageFrame::getContentBounds() const
{
    using namespace StageMetrics;
    return getLocalBounds().toFloat()
               .withTrimmedTop (scaled (pagePaddingTop) + titleHeight() + scaled (pageRowGap))
               .withTrimmedBottom (scaled (pagePaddingTop))
               .reduced (scaled (pagePaddingSide), float {})
               .toNearestInt();
}

void StagePageFrame::resized()
{
    if (onLayoutContent != nullptr)
        onLayoutContent (getContentBounds(), scale);
}

void StagePageFrame::paint (juce::Graphics& g)
{
    using namespace StageMetrics;
    const auto bounds = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient::vertical (StagePalette::pageTop, bounds.getY(), StagePalette::pageBottom, bounds.getBottom()));
    g.fillRect (bounds);
    g.setColour (StagePalette::panelBottomEdge);
    g.fillRect (bounds.withTop (bounds.getBottom() - scaled (pageBottomEdge)));

    // Title row: the page name in silkscreen, then a rule to the right edge.
    const auto font = StageFonts::silkscreen (pageTitleSize * scale, semiBold, pageTitleSpacing);
    const auto titleText = title.toUpperCase();
    const auto titleRow = bounds.withTrimmedTop (scaled (pagePaddingTop)).withHeight (titleHeight())
                                .reduced (scaled (pagePaddingSide), float {});
    const auto titleWidth = std::ceil (juce::GlyphArrangement::getStringWidth (font, titleText));   // letter-spacing included, as in CSS
    g.setFont (font);
    g.setColour (StagePalette::silkscreenText);
    g.drawText (titleText, titleRow.withWidth (titleWidth), juce::Justification::centredLeft, false);

    const auto ruleLeft = titleRow.getX() + titleWidth + scaled (pageTitleRuleGap);
    g.setColour (StagePalette::ruleLine);
    g.fillRect (juce::Rectangle<float> (ruleLeft, titleRow.getCentreY() - scaled (ruleWidth) / halves,
                                        titleRow.getRight() - ruleLeft, scaled (ruleWidth)));
}
