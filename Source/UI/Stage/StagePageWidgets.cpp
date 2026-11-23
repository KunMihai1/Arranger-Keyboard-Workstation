#include "StagePageWidgets.h"
#include <cmath>
#include "StageFonts.h"
#include "StageWidgets.h"
#include "../../Common/AppColours.h"

namespace
{
    constexpr bool  semiBold        = true;
    constexpr bool  medium          = false;
    constexpr float halves          = 2.0f;
    constexpr float disabledAlpha   = 0.4f;
    constexpr float silkBrightSpacing = 0.12f;   // .silk letter-spacing
    constexpr float wholeAlpha      = 1.0f;      // the shadow's alpha, shared out over its stops
    constexpr double faderLowest    = 0.0;
    constexpr double faderHighest   = 127.0;     // MIDI CC7
    constexpr double faderStep      = 1.0;
    // .fader-cap gradient stops: #5b5c60 0, #3a3b3e 45 %, #1c1d1f 50 %, #3a3b3e 55 %, #2c2d30 100 %
    constexpr double capMidStop     = 0.45;
    constexpr double capGrooveStop  = 0.5;
    constexpr double capLowStop     = 0.55;
    constexpr int    capShadowStops = 4;
    constexpr int    firstShadowStep = 1;
}

//==============================================================================
SegButton::SegButton (const juce::String& text, Style buttonStyle, bool withLed)
    : juce::Button (text),
      style (buttonStyle),
      hasLed (withLed),
      boardHeight (buttonStyle == Style::instrument ? StageMetrics::instrumentHeight : StageMetrics::segHeight)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);   // owner: every button shows the hand
}

void SegButton::setLedOn (bool shouldBeOn)
{
    if (ledOn != shouldBeOn)
    {
        ledOn = shouldBeOn;
        repaint();
    }
}

bool SegButton::isLedOn() const { return ledOn; }
SegButton::Style SegButton::getStyle() const { return style; }

void SegButton::setScale (float newScale)
{
    scale = newScale;
    repaint();
}

void SegButton::setBoardHeight (int newBoardHeight)
{
    boardHeight = newBoardHeight;
    repaint();
}

void SegButton::setTextSize (float newCssPixelSize)
{
    textSize = newCssPixelSize;
    repaint();
}

void SegButton::setPaddingSide (int newBoardPadding)
{
    paddingSide = newBoardPadding;
    repaint();
}

float SegButton::scaled (float referenceLength) const { return referenceLength * scale; }
float SegButton::scaled (int referenceLength) const   { return static_cast<float> (referenceLength) * scale; }

juce::Font SegButton::labelFont() const
{
    const auto spacing = style == Style::instrument ? StageMetrics::instrumentSpacing : StageMetrics::segLabelSpacing;
    return StageFonts::silkscreen (scaled (textSize), semiBold, spacing);
}

juce::String SegButton::shownText() const
{
    // .seg is text-transform: uppercase; the instrument button shows the name as it is.
    return style == Style::instrument ? getButtonText() : getButtonText().toUpperCase();
}

int SegButton::getPreferredWidth() const
{
    using namespace StageMetrics;
    if (style == Style::instrument)
        return juce::roundToInt (scaled (instrumentWidth));

    auto width = juce::GlyphArrangement::getStringWidth (labelFont(), shownText()) + scaled (paddingSide + segBorder) * halves;
    if (hasLed)
        width += scaled (ledDiameter + segLedGap);
    return juce::roundToInt (width);
}

int SegButton::getPreferredHeight() const
{
    return juce::roundToInt (scaled (boardHeight));
}

juce::Rectangle<float> SegButton::getLedBounds() const
{
    using namespace StageMetrics;
    const auto diameter = scaled (ledDiameter);
    const auto face     = getLocalBounds().toFloat().withHeight (scaled (boardHeight));
    return { face.getX() + scaled (segBorder + paddingSide), face.getCentreY() - diameter / halves, diameter, diameter };
}

void SegButton::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    juce::ignoreUnused (isHighlighted);
    using namespace StageMetrics;

    const bool lit    = style == Style::lit;
    const auto border = scaled (segBorder);
    auto face = getLocalBounds().toFloat().withHeight (juce::jmin (scaled (boardHeight), static_cast<float> (getHeight())));

    const auto top    = lit ? StagePalette::litTop : StagePalette::rubberTop;
    const auto bottom = lit ? StagePalette::litBottom : StagePalette::rubberBottom;

    if (style == Style::instrument)
    {
        // .inst: rounded, with a 2 px drop edge under it (box-shadow 0 2px 0 #0d0e0f); pressed, it sits down on it.
        const auto corner = scaled (instrumentCorner);
        const auto drop   = scaled (instrumentDropEdge);
        if (isDown)
            face.translate (float {}, drop / halves);
        g.setColour (StagePalette::rubberDropEdge);
        g.fillRoundedRectangle (face.translated (float {}, isDown ? drop / halves : drop), corner);
        g.setGradientFill (juce::ColourGradient::vertical (top, face.getY(), bottom, face.getBottom()));
        g.fillRoundedRectangle (face, corner);
        g.setColour (StagePalette::rubberBorder);
        g.drawRoundedRectangle (face.reduced (border / halves), corner, border);
    }
    else
    {
        // .seg: square, flat gradient, 1 px border; pressed = the lower half's colour all over.
        if (isDown)
            g.setColour (bottom);
        else
            g.setGradientFill (juce::ColourGradient::vertical (top, face.getY(), bottom, face.getBottom()));
        g.fillRect (face);
        g.setColour (lit ? StagePalette::litBorder : StagePalette::rubberBorder);
        g.drawRect (face, border);
    }

    auto textArea = face.reduced (style == Style::instrument ? scaled (instrumentPadding) : scaled (segBorder + paddingSide), float {});
    if (hasLed)
    {
        StageDrawing::drawLed (g, getLedBounds(), ledOn, StagePalette::ledRed, scale);
        textArea.removeFromLeft (scaled (ledDiameter + segLedGap));
    }

    auto colour = lit ? StagePalette::litText : StagePalette::silkscreenText;
    if (! isEnabled())
        colour = colour.withAlpha (disabledAlpha);
    g.setColour (colour);
    g.setFont (labelFont());
    g.drawText (shownText(), textArea, juce::Justification::centred, style == Style::instrument);
}

//==============================================================================
SilkLabel::SilkLabel (const juce::String& labelText, Tone labelTone)
    : text (labelText),
      tone (labelTone),
      letterSpacing (labelTone == Tone::bright ? silkBrightSpacing : StageMetrics::silkDimSpacing)
{
    setInterceptsMouseClicks (false, false);
}

void SilkLabel::setText (const juce::String& newText)
{
    if (text != newText)
    {
        text = newText;
        repaint();
    }
}

juce::String SilkLabel::getText() const { return text; }

void SilkLabel::setScale (float newScale)
{
    scale = newScale;
    repaint();
}

void SilkLabel::setTextStyle (float newCssPixelSize, float newLetterSpacingEm)
{
    textSize      = newCssPixelSize;
    letterSpacing = newLetterSpacingEm;
    repaint();
}

juce::Font SilkLabel::font() const
{
    return StageFonts::silkscreen (textSize * scale, tone == Tone::bright ? semiBold : medium, letterSpacing);
}

int SilkLabel::getPreferredWidth() const
{
    return juce::roundToInt (std::ceil (juce::GlyphArrangement::getStringWidth (font(), text.toUpperCase())));
}

void SilkLabel::paint (juce::Graphics& g)
{
    g.setColour (tone == Tone::bright ? StagePalette::silkscreenText : StagePalette::silkscreenDimText);
    g.setFont (font());
    g.drawText (text.toUpperCase(), getLocalBounds().toFloat(), juce::Justification::centredLeft, false);
}

//==============================================================================
TitleDivider::TitleDivider()
{
    setInterceptsMouseClicks (false, false);
}

void TitleDivider::paint (juce::Graphics& g)
{
    // The owner sizes it: the rule's width (1 board px, scaled) by the divider's height.
    g.setColour (StagePalette::ruleLine);
    g.fillRect (getLocalBounds());
}

//==============================================================================
StageFader::StageFader()
{
    setSliderStyle (juce::Slider::LinearVertical);
    setTextBoxStyle (juce::Slider::NoTextBox, false, {}, {});
    setRange (faderLowest, faderHighest, faderStep);
    setSliderSnapsToMousePosition (false);   // a real fader moves by the drag, it never jumps to the click
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void StageFader::setScale (float newScale)
{
    scale = newScale;
    repaint();
}

float StageFader::scaled (float referenceLength) const { return referenceLength * scale; }
float StageFader::scaled (int referenceLength) const   { return static_cast<float> (referenceLength) * scale; }

float StageFader::capTopFor (double value)
{
    const auto travel = static_cast<double> (StageMetrics::faderCapTravel);
    return static_cast<float> (travel - travel * value / faderHighest);
}

void StageFader::paint (juce::Graphics& g)
{
    using namespace StageMetrics;
    // The board's 40 x 132 fader, centred in the component.
    const auto box = getLocalBounds().toFloat().withSizeKeepingCentre (scaled (faderWidth), scaled (faderHeight));

    g.setColour (StagePalette::faderScale);
    for (int mark {}; mark < faderScaleMarks; ++mark)
        g.fillRect (box.getX() + scaled (faderScaleLeft), box.getY() + scaled (faderSlotTop + mark * faderScaleStep),
                    scaled (faderScaleWidth), scaled (ruleWidth));

    const juce::Rectangle<float> slot { box.getX() + scaled (faderSlotLeft), box.getY() + scaled (faderSlotTop),
                                        scaled (faderSlotWidth), scaled (faderSlotHeight) };
    const auto slotCorner = scaled (faderSlotCorner);
    g.setColour (StagePalette::faderSlotShine);
    g.fillRoundedRectangle (slot.translated (float {}, scaled (ruleWidth)), slotCorner);
    g.setColour (StagePalette::faderSlot);
    g.fillRoundedRectangle (slot, slotCorner);
    g.setGradientFill (juce::ColourGradient::vertical (StagePalette::faderSlotShade, slot.getY(),
                                                       juce::Colours::transparentBlack, slot.getY() + scaled (ruleWidth)));
    g.fillRoundedRectangle (slot, slotCorner);

    const juce::Rectangle<float> cap { box.getX() + scaled (faderCapLeft), box.getY() + capTopFor (getValue()) * scale,
                                       scaled (faderCapWidth), scaled (faderCapHeight) };
    const auto capCorner = scaled (faderCapCorner);

    // box-shadow 0 3px 4px rgba(0,0,0,.6): a soft shadow under the cap, fading over the blur.
    const auto blur = scaled (faderCapShadowBlur);
    for (int step = firstShadowStep; step <= capShadowStops; ++step)
    {
        const auto spread = blur * static_cast<float> (step) / static_cast<float> (capShadowStops);
        g.setColour (StagePalette::faderCapShadow.withMultipliedAlpha (wholeAlpha / static_cast<float> (capShadowStops)));
        g.fillRoundedRectangle (cap.translated (float {}, scaled (faderCapShadowY)).expanded (spread / halves), capCorner + spread / halves);
    }

    juce::ColourGradient body (StagePalette::faderCapTop, cap.getX(), cap.getY(), StagePalette::faderCapBottom, cap.getX(), cap.getBottom(), false);
    body.addColour (capMidStop,    StagePalette::faderCapMid);
    body.addColour (capGrooveStop, StagePalette::faderCapGroove);
    body.addColour (capLowStop,    StagePalette::faderCapMid);
    g.setGradientFill (body);
    g.fillRoundedRectangle (cap, capCorner);
    g.setColour (StagePalette::faderCapShine);
    g.fillRect (cap.reduced (capCorner, float {}).withHeight (scaled (ruleWidth)));
}

//==============================================================================
namespace StageLcd
{
    juce::Colour textColour (bool selected)
    {
        return selected ? StagePalette::lcd : StagePalette::lcdInk;
    }

    juce::Font rowFont (float scale)
    {
        return StageFonts::lcd (StageMetrics::lcdListTextSize * scale);
    }

    void paintPanel (juce::Graphics& g, juce::Rectangle<float> area, float scale)
    {
        const auto corner = static_cast<float> (StageMetrics::lcdListCorner) * scale;
        g.setColour (StagePalette::lcd);
        g.fillRoundedRectangle (area, corner);
        // inset 0 1px 3px rgba(0,0,0,.5): darkest at the top edge, gone a few px in.
        const auto shade = static_cast<float> (StageMetrics::lcdInsetShadow + StageMetrics::ruleWidth) * scale;
        g.setGradientFill (juce::ColourGradient::vertical (StagePalette::lcdInset, area.getY(),
                                                           juce::Colours::transparentBlack, area.getY() + shade));
        g.fillRoundedRectangle (area.withHeight (shade), corner);
    }

    void paintRow (juce::Graphics& g, juce::Rectangle<int> row, float scale,
                   const juce::String& leftText, const juce::String& rightText, bool selected)
    {
        g.setColour (selected ? StagePalette::lcdInk : StagePalette::lcd);
        g.fillRect (row);

        const auto text = row.toFloat().reduced (static_cast<float> (StageMetrics::lcdRowPaddingSide) * scale, float {});
        g.setColour (textColour (selected));
        g.setFont (rowFont (scale));
        g.drawText (leftText, text, juce::Justification::centredLeft, true);
        if (rightText.isNotEmpty())
            g.drawText (rightText, text, juce::Justification::centredRight, false);
    }

    juce::Rectangle<float> headingRule (juce::Rectangle<float> area, float scale,
                                        const juce::String& title, const juce::String& dimText)
    {
        const auto gap   = static_cast<float> (StageMetrics::headingGap) * scale;
        const auto left  = area.getX() + juce::GlyphArrangement::getStringWidth (StageFonts::silkscreen (StageMetrics::silkLabelSize * scale, semiBold, silkBrightSpacing), title.toUpperCase()) + gap;
        auto right = area.getRight();
        if (dimText.isNotEmpty())
            right -= juce::GlyphArrangement::getStringWidth (StageFonts::silkscreen (StageMetrics::silkLabelSize * scale, medium, StageMetrics::silkDimSpacing), dimText.toUpperCase()) + gap;
        const auto thickness = static_cast<float> (StageMetrics::ruleWidth) * scale;
        return { left, area.getCentreY() - thickness / halves, juce::jmax (float {}, right - left), thickness };
    }

    void paintHeading (juce::Graphics& g, juce::Rectangle<float> area, float scale,
                       const juce::String& title, const juce::String& dimText)
    {
        g.setColour (StagePalette::silkscreenText);
        g.setFont (StageFonts::silkscreen (StageMetrics::silkLabelSize * scale, semiBold, silkBrightSpacing));
        g.drawText (title.toUpperCase(), area, juce::Justification::centredLeft, false);

        g.setColour (StagePalette::ruleLine);
        g.fillRect (headingRule (area, scale, title, dimText));

        if (dimText.isNotEmpty())
        {
            g.setColour (StagePalette::silkscreenDimText);
            g.setFont (StageFonts::silkscreen (StageMetrics::silkLabelSize * scale, medium, StageMetrics::silkDimSpacing));
            g.drawText (dimText.toUpperCase(), area, juce::Justification::centredRight, false);
        }
    }
}
