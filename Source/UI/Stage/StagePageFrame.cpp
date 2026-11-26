#include "StagePageFrame.h"
#include <cmath>
#include "StageFonts.h"
#include "../../Common/AppColours.h"

namespace
{
    constexpr bool  semiBold = true;
    constexpr bool  medium   = false;
    constexpr float halves   = 2.0f;
    constexpr float onePixel = 1.0f;
}

StagePageFrame::StagePageFrame (const juce::String& pageTitle)
    : title (pageTitle)
{
    setOpaque (true);
    exitButton.onClick = [this] { if (onExit != nullptr) onExit(); };
    addAndMakeVisible (exitButton);
}

void StagePageFrame::setScale (float newScale)
{
    scale = newScale;
    exitButton.setScale (scale);
    for (auto& action : actions)
    {
        if (auto* seg = dynamic_cast<SegButton*> (action.getComponent()))
            seg->setScale (scale);
        else if (auto* label = dynamic_cast<SilkLabel*> (action.getComponent()))
            label->setScale (scale);
    }
    resized();
    repaint();
}

void StagePageFrame::setTitle (const juce::String& newTitle)
{
    title = newTitle;
    resized();
    repaint();
}

void StagePageFrame::setSubtitle (const juce::String& newSubtitle)
{
    subtitle = newSubtitle;
    repaint();
}

void StagePageFrame::setActions (std::vector<juce::Component*> leftToRight)
{
    for (auto& action : actions)
        if (action != nullptr && action->getParentComponent() == this)
            removeChildComponent (action.getComponent());

    actions.clear();
    for (auto* action : leftToRight)
    {
        actions.emplace_back (action);
        addAndMakeVisible (action);
    }
    setScale (scale);
}

void StagePageFrame::setRowGap (int newBoardGap)
{
    rowGap = newBoardGap;
    resized();
    repaint();
}

SegButton& StagePageFrame::getExitButton()                 { return exitButton; }
juce::String StagePageFrame::getTitle() const              { return title; }
float StagePageFrame::scaled (int referenceLength) const   { return static_cast<float> (referenceLength) * scale; }

juce::Font StagePageFrame::titleFont() const
{
    return StageFonts::silkscreen (StageMetrics::pageTitleSize * scale, semiBold, StageMetrics::pageTitleSpacing);
}

juce::Font StagePageFrame::subtitleFont() const
{
    return StageFonts::silkscreen (StageMetrics::silkLabelSize * scale, medium, StageMetrics::silkDimSpacing);
}

juce::Rectangle<float> StagePageFrame::boardBand() const
{
    // Spec 4.1: the board scaled as one piece and centred; a wider window only adds plain page at the sides.
    const auto bounds = getLocalBounds().toFloat();
    return bounds.withSizeKeepingCentre (juce::jmin (bounds.getWidth(), scaled (StageMetrics::referenceWidth)), bounds.getHeight());
}

juce::Rectangle<float> StagePageFrame::titleRow() const
{
    using namespace StageMetrics;
    // The title row is as tall as its buttons (.seg, 22): the title is centred in it.
    return boardBand()
               .withTrimmedTop (scaled (pagePaddingTop))
               .withHeight (scaled (segHeight))
               .reduced (scaled (pagePaddingSide), float {});
}

juce::Rectangle<int> StagePageFrame::getContentBounds() const
{
    using namespace StageMetrics;
    return boardBand()
               .withTrimmedTop (scaled (pagePaddingTop + segHeight + rowGap))
               .withTrimmedBottom (scaled (pagePaddingTop + pageBottomEdge))
               .reduced (scaled (pagePaddingSide), float {})
               .toNearestInt();
}

juce::Rectangle<float> StagePageFrame::getTitleBounds() const
{
    const auto width = std::ceil (juce::GlyphArrangement::getStringWidth (titleFont(), title.toUpperCase()));   // letter-spacing included, as in CSS
    return titleRow().withWidth (width);
}

juce::Rectangle<float> StagePageFrame::getSubtitleBounds() const
{
    const auto titleBounds = getTitleBounds();
    if (subtitle.isEmpty())
        return titleBounds.withX (titleBounds.getRight()).withWidth (float {});
    const auto width = std::ceil (juce::GlyphArrangement::getStringWidth (subtitleFont(), subtitle.toUpperCase()));
    return titleBounds.withX (titleBounds.getRight() + scaled (rowGap)).withWidth (width);
}

float StagePageFrame::actionWidth (juce::Component& action) const
{
    if (auto* seg = dynamic_cast<SegButton*> (&action))
        return seg->getExactWidth();
    if (auto* label = dynamic_cast<SilkLabel*> (&action))
        return label->getExactWidth();
    if (dynamic_cast<TitleDivider*> (&action) != nullptr)
        return juce::jmax (scaled (StageMetrics::ruleWidth), onePixel);   // never thinner than a pixel
    return static_cast<float> (action.getWidth());
}

float StagePageFrame::ruleRight() const
{
    auto right = static_cast<float> (exitButton.getX());
    for (auto& action : actions)
        if (action != nullptr)
            right = juce::jmin (right, static_cast<float> (action->getX()));
    return right - scaled (rowGap);
}

void StagePageFrame::resized()
{
    using namespace StageMetrics;
    const auto row = titleRow();
    const auto gap = scaled (rowGap);

    // Right to left: Exit, then the actions in reverse order, each one gap apart.
    auto right = row.getRight();
    const auto exitWidth = exitButton.getExactWidth();
    exitButton.setBounds (juce::Rectangle<float> (right - exitWidth, row.getY(), exitWidth, row.getHeight()).toNearestInt());
    right -= exitWidth + gap;

    for (auto action = actions.rbegin(); action != actions.rend(); ++action)
    {
        auto* component = action->getComponent();
        if (component == nullptr)
            continue;
        const auto width = actionWidth (*component);
        auto box = juce::Rectangle<float> (right - width, row.getY(), width, row.getHeight());
        if (dynamic_cast<TitleDivider*> (component) != nullptr)
            box = box.withSizeKeepingCentre (width, scaled (dividerHeight));
        // A label rounds outwards: a box rounded narrower than its text loses the last letters (JUCE drops glyphs that don't fit).
        const bool isLabel = dynamic_cast<SilkLabel*> (component) != nullptr;
        component->setBounds (isLabel ? box.getSmallestIntegerContainer() : box.toNearestInt());
        right -= width + gap;
    }

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

    // Title row: the page name in silkscreen, the dim subtitle, then a rule up to the actions.
    g.setFont (titleFont());
    g.setColour (StagePalette::silkscreenText);
    g.drawText (title.toUpperCase(), getTitleBounds(), juce::Justification::centredLeft, false);

    const auto subtitleBounds = getSubtitleBounds();
    if (subtitle.isNotEmpty())
    {
        g.setFont (subtitleFont());
        g.setColour (StagePalette::silkscreenDimText);
        g.drawText (subtitle.toUpperCase(), subtitleBounds, juce::Justification::centredLeft, false);
    }

    const auto row      = titleRow();
    const auto ruleLeft = subtitleBounds.getRight() + scaled (rowGap);
    const auto right    = ruleRight();
    if (right > ruleLeft)
    {
        g.setColour (StagePalette::ruleLine);
        g.fillRect (juce::Rectangle<float> (ruleLeft, row.getCentreY() - scaled (ruleWidth) / halves, right - ruleLeft, scaled (ruleWidth)));
    }
}
