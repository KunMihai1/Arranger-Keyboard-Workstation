#include "SectionLaneHeaders.h"
#include "Common/AppColours.h"
#include "UI/Stage/StageFonts.h"

namespace
{
    constexpr float buttonTextSize    = 9.0f;    // .ntt: 9px, letter-spacing .08em, uppercase
    constexpr float buttonSpacing     = 0.08f;
    constexpr bool  semiBold          = true;
    constexpr int   buttonPaddingX    = 5;       // .ntt padding 1px 5px, 1px border
    constexpr int   buttonPaddingY    = 1;
    constexpr int   buttonBorder      = 1;
    constexpr float buttonLineHeight  = 1.2f;    // CSS line-height: normal
    constexpr int   twice             = 2;
    constexpr int   firstNumber       = 1;       // lanes read "1 Drums"
    constexpr float corner            = 2.0f;
    constexpr float rowRule           = 1.0f;    // .lane-head border-bottom: 1px
    const juce::String numberSeparator = " ";
    const char* const emptySlotUtf8    = "\xe2\x80\x94";   // "—"
    const juce::String trackPrefix     = "Track ";
}

SectionLaneHeaders::SectionLaneHeaders()
{
    setInterceptsMouseClicks (true, false);
}

void SectionLaneHeaders::setTracks (const std::vector<Track>& newTracks)
{
    tracks = newTracks;
    repaint();
}

void SectionLaneHeaders::setScale (float newScale)
{
    scale = newScale;
    repaint();
}

juce::Font SectionLaneHeaders::buttonFont() const
{
    return StageFonts::silkscreen (buttonTextSize * scale, semiBold, buttonSpacing);
}

juce::String SectionLaneHeaders::laneLabel (int laneIndex) const
{
    const auto number = juce::String (laneIndex + firstNumber) + numberSeparator;
    if (! juce::isPositiveAndBelow (laneIndex, (int) tracks.size()))
        return number + juce::String::fromUTF8 (emptySlotUtf8);
    const auto& name = tracks[(size_t) laneIndex].name;
    return number + (name.isNotEmpty() ? name : trackPrefix + juce::String (laneIndex + firstNumber));
}

juce::Rectangle<int> SectionLaneHeaders::buttonBounds (int trackIndex) const
{
    if (! juce::isPositiveAndBelow (trackIndex, (int) tracks.size()))
        return {};
    const auto lane = SectionLanes::laneBounds (trackIndex, getWidth(), scale);
    const auto text = SectionLanes::transposeModeText (tracks[(size_t) trackIndex].mode).toUpperCase();
    const float width  = juce::GlyphArrangement::getStringWidth (buttonFont(), text) + (float) (twice * (buttonPaddingX + buttonBorder)) * scale;
    const float height = (buttonTextSize * buttonLineHeight + (float) (twice * (buttonPaddingY + buttonBorder))) * scale;
    const float right  = (float) lane.getRight() - (float) SectionLaneMetrics::headerPaddingX * scale;
    return juce::Rectangle<float> (right - width, lane.toFloat().getCentreY() - height / twice, width, height).getSmallestIntegerContainer()
               .withRight (juce::roundToInt (right));
}

void SectionLaneHeaders::clickAt (juce::Point<int> position)
{
    for (int trackIndex {}; trackIndex < (int) tracks.size(); ++trackIndex)
    {
        if (! buttonBounds (trackIndex).contains (position))
            continue;
        auto& track = tracks[(size_t) trackIndex];
        track.mode = SectionLanes::nextTransposeMode (track.mode);
        repaint();
        if (onModeChanged)
            onModeChanged (trackIndex, track.mode);
        return;
    }
}

void SectionLaneHeaders::mouseDown (const juce::MouseEvent& event)
{
    clickAt (event.getPosition());
}

void SectionLaneHeaders::paint (juce::Graphics& g)
{
    using namespace SectionLaneMetrics;
    const int lanes = SectionLanes::laneCount ((int) tracks.size());
    for (int laneIndex {}; laneIndex < lanes; ++laneIndex)
    {
        const auto lane = SectionLanes::laneBounds (laneIndex, getWidth(), scale).toFloat();
        g.setColour (StagePalette::stripBorder);
        g.fillRect (lane.withTop (lane.getBottom() - rowRule));

        const auto button = buttonBounds (laneIndex).toFloat();
        g.setColour (StagePalette::silkscreenText);
        g.setFont (StageFonts::lcd (laneNameSize * scale));
        const auto nameArea = lane.withTrimmedLeft ((float) headerPaddingX * scale)
                                  .withRight (button.isEmpty() ? lane.getRight() : button.getX() - (float) buttonPaddingX * scale);
        g.drawText (laneLabel (laneIndex), nameArea, juce::Justification::centredLeft, true);

        if (button.isEmpty())
            continue;
        g.setColour (StagePalette::transposeChipBorder);
        g.drawRoundedRectangle (button.reduced ((float) buttonBorder * scale / twice), corner * scale, (float) buttonBorder * scale);
        g.setColour (StagePalette::silkscreenDimText);
        g.setFont (buttonFont());
        g.drawText (SectionLanes::transposeModeText (tracks[(size_t) laneIndex].mode).toUpperCase(), button, juce::Justification::centred, false);
    }
}
