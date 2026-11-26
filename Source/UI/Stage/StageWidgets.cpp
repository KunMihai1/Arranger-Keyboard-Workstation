#include "StageWidgets.h"
#include <cmath>
#include "StageFonts.h"
#include "../../Common/AppColours.h"

namespace
{
    // Drawing details of the board's CSS (StageRevised .btn / .led / .qknob), reference px.
    constexpr float capCornerRadius   = 3.0f;
    constexpr float capBorderWidth    = 1.0f;
    constexpr float capHighlightWidth = 1.0f;
    constexpr float pressedTravel     = 1.0f;   // the cap moves down 1 px and its drop edge shrinks by 1
    constexpr float ledShadeAlpha     = 0.6f;   // inset shadow at the top of an unlit LED
    constexpr float ledShadeEnd       = 0.6f;   // ... fading out by 60 % of its height
    constexpr float cssBlurSigmaPerRadius = 0.5f;   // CSS: a blur radius B is a Gaussian with sigma B / 2
    constexpr float blurredEdgeCoverage   = 0.5f;   // a blurred shape is half covered where it ends
    constexpr int   glowStops             = 8;      // gradient stops tracing the blur's fall-off
    constexpr float knobShadowAlpha   = 0.55f;
    constexpr float knobOutlineWidth  = 1.0f;
    constexpr float capGradientCentreX = 0.38f;
    constexpr float capGradientCentreY = 0.30f;
    constexpr float capGradientMidStop = 0.55f;
    constexpr float capHighlightAlpha  = 0.14f;
    constexpr float capHighlightArcDegrees = 60.0f;   // each side of 12 o'clock
    constexpr float knobRingOffsetY    = 0.0f;
    constexpr float halves             = 2.0f;
    constexpr bool  semiBold           = true;
    constexpr bool  medium             = false;
    const juce::String lineBreak       = "\n";
    const juce::String space           = " ";
    constexpr int skirtColourCount     = 2;   // the knurl alternates dark / light
    constexpr double knobLowest        = 0.0;
    constexpr double knobHighest       = 127.0;   // MIDI controller range
    constexpr double knobStep          = 1.0;
    constexpr double fullTurnProportion = 1.0;   // the whole 270-degree sweep
    const juce::Colour outlineColour   { 0xff050505 };

    struct KnobGeometry
    {
        int cellWidth, ring, skirt, cap, tickLength, pointerTop, pointerLength;
        int shadowOffset, shadowBlur, segmentDegrees, labelGap;
    };

    KnobGeometry geometryFor (StageKnob::Size size)
    {
        using namespace StageMetrics;
        if (size == StageKnob::Size::effects)
            return { effectsKnobCellWidth, effectsKnobRing, effectsKnobSkirt, effectsKnobCap, effectsKnobTickLength,
                     effectsKnobPointerTop, effectsKnobPointerLength, effectsKnobShadowOffset, effectsKnobShadowBlur,
                     effectsKnobSegmentDegrees, effectsKnobLabelGap };

        return { quickKnobCellWidth, quickKnobRing, quickKnobSkirt, quickKnobCap, quickKnobTickLength,
                 quickKnobPointerTop, quickKnobPointerLength, quickKnobShadowOffset, quickKnobShadowBlur,
                 quickKnobSegmentDegrees, quickKnobLabelGap };
    }

    juce::Colour colourOf (RubberButton::LedColour ledColour)
    {
        return ledColour == RubberButton::LedColour::green ? StagePalette::ledGreen : StagePalette::ledRed;
    }

}

void StageDrawing::drawLed (juce::Graphics& g, juce::Rectangle<float> led, bool lit, juce::Colour litColour, float scale)
{
    if (lit)
    {
        // .led-on box-shadow 0 0 3px: the LED blurred by a Gaussian, so the glow starts at half
        // strength at the LED's edge and has all but gone by the blur radius.
        const auto blur     = StageMetrics::ledGlowRadius * scale;
        const auto sigma    = blur * cssBlurSigmaPerRadius;
        const auto radius   = led.getWidth() / halves;
        const auto glowArea = led.expanded (blur);
        auto glowAt = [&] (float gap)
        {
            const auto coverage = blurredEdgeCoverage * static_cast<float> (std::erfc (gap / (sigma * juce::MathConstants<float>::sqrt2)));
            return litColour.withAlpha (StagePalette::ledGlowAlpha * coverage);
        };
        juce::ColourGradient glow (glowAt (float {}), led.getCentre(), glowAt (blur), { led.getCentreX(), glowArea.getY() }, true);
        for (int stop {}; stop <= glowStops; ++stop)
        {
            const auto gap = blur * static_cast<float> (stop) / static_cast<float> (glowStops);
            glow.addColour ((radius + gap) / (radius + blur), glowAt (gap));
        }
        g.setGradientFill (glow);
        g.fillEllipse (glowArea);
        g.setColour (litColour);
        g.fillEllipse (led);
        return;
    }

    g.setColour (StagePalette::ledOff);
    g.fillEllipse (led);
    g.setGradientFill (juce::ColourGradient::vertical (juce::Colours::black.withAlpha (ledShadeAlpha), led.getY(),
                                                      juce::Colours::transparentBlack, led.getY() + led.getHeight() * ledShadeEnd));
    g.fillEllipse (led);
}

namespace
{
    void drawCap (juce::Graphics& g, juce::Rectangle<float> cap, bool down, float scale)
    {
        const auto travel = down ? pressedTravel * scale : float {};
        const auto drop   = StageMetrics::buttonDropEdge * scale - travel;
        const auto corner = capCornerRadius * scale;
        const auto face   = cap.translated (float {}, travel);

        g.setColour (StagePalette::rubberDropEdge);
        g.fillRoundedRectangle (face.translated (float {}, drop), corner);

        g.setGradientFill (juce::ColourGradient::vertical (StagePalette::rubberTop, face.getY(), StagePalette::rubberBottom, face.getBottom()));
        g.fillRoundedRectangle (face, corner);

        const auto border = capBorderWidth * scale;
        g.setColour (StagePalette::rubberHighlight);
        g.fillRect (face.reduced (corner, float {}).withY (face.getY() + border).withHeight (capHighlightWidth * scale));

        g.setColour (StagePalette::rubberBorder);
        g.drawRoundedRectangle (face.reduced (border / halves), corner, border);
    }

    void drawLabelLines (juce::Graphics& g, const juce::StringArray& lines, juce::Rectangle<float> area,
                         float lineHeight, juce::Justification justification)
    {
        for (const auto& line : lines)
        {
            g.drawText (line.toUpperCase(), area.removeFromTop (lineHeight), justification, false);
        }
    }
}

//==============================================================================
RubberButton::RubberButton (const juce::String& labelText, Style buttonStyle, LedColour buttonLedColour)
    : juce::Button (labelText),
      label (labelText),
      style (buttonStyle),
      ledColour (buttonLedColour)
{
    labelLines.addTokens (label, lineBreak, {});
    setTitle (label.replace (lineBreak, space));
    setMouseCursor (juce::MouseCursor::PointingHandCursor);   // owner: every button shows the hand
}

void RubberButton::setLedOn (bool shouldBeOn)
{
    if (ledOn != shouldBeOn)
    {
        ledOn = shouldBeOn;
        repaint();
    }
}

bool RubberButton::isLedOn() const                      { return ledOn; }
void RubberButton::setScale (float newScale)             { scale = newScale; repaint(); }
void RubberButton::setLabelTone (LabelTone newTone)      { labelTone = newTone; repaint(); }
void RubberButton::setLabelSide (LabelSide newSide)      { labelSide = newSide; repaint(); }
RubberButton::Style RubberButton::getStyle() const       { return style; }
RubberButton::LedColour RubberButton::getLedColour() const { return ledColour; }
juce::String RubberButton::getLabel() const              { return label; }
int RubberButton::getLabelLineCount() const              { return labelLines.size(); }
float RubberButton::scaled (float referenceLength) const { return referenceLength * scale; }
float RubberButton::scaled (int referenceLength) const   { return scaled (static_cast<float> (referenceLength)); }

juce::Font RubberButton::getLabelFont() const
{
    const auto isKey = style == Style::key;
    return StageFonts::silkscreen (scaled (isKey ? StageMetrics::keyLabelSize : StageMetrics::silkLabelSize),
                                   labelTone == LabelTone::bright ? semiBold : medium,
                                   isKey ? StageMetrics::keyLabelSpacing : StageMetrics::silkLabelSpacing);
}

float RubberButton::labelWidth() const
{
    const auto font = getLabelFont();
    float widest {};
    for (const auto& line : labelLines)
        widest = juce::jmax (widest, juce::GlyphArrangement::getStringWidth (font, line.toUpperCase()));

    // The measured width already holds the letter-spacing after every letter, the last one too (as in
    // CSS). A label cut by rounding loses its last glyph, so round up (seen: "LEFT" drawn as "LEF").
    return std::ceil (widest);
}

int RubberButton::glowRoom() const
{
    return static_cast<int> (std::ceil (scaled (StageMetrics::ledGlowRadius)));
}

juce::Rectangle<int> RubberButton::boundsForBoardBox (juce::Rectangle<int> boardBox) const
{
    return boardBox.expanded (glowRoom());
}

juce::Rectangle<float> RubberButton::getBoardBox() const
{
    return getLocalBounds().reduced (glowRoom()).toFloat();
}

int RubberButton::getPreferredWidth() const
{
    using namespace StageMetrics;
    switch (style)
    {
        case Style::key:       return juce::roundToInt (scaled (keyComponentWidth));
        case Style::screen:    return juce::roundToInt (labelWidth() + scaled (screenButtonGap + screenButtonWidth + screenButtonGap + ledDiameter));
        // Rounded up: a box rounded narrower than its label loses the last letter ("RIGH" at some window sizes).
        case Style::ledToggle: return static_cast<int> (std::ceil (scaled (ledDiameter + toggleGap) + labelWidth()));
    }
    return {};
}

int RubberButton::getPreferredHeight() const
{
    using namespace StageMetrics;
    switch (style)
    {
        case Style::key:       return juce::roundToInt (scaled (buttonHeight + buttonToLabelGap + keyLabelLineHeight * getLabelLineCount()));
        case Style::screen:    return juce::roundToInt (scaled (screenButtonHeight));
        case Style::ledToggle: return juce::roundToInt (scaled (toggleHeight));
    }
    return {};
}

juce::Rectangle<float> RubberButton::getCapBounds() const
{
    using namespace StageMetrics;
    const auto bounds = getBoardBox();
    switch (style)
    {
        case Style::key:
            return { bounds.getCentreX() - scaled (buttonWidth) / halves, bounds.getY(), scaled (buttonWidth), scaled (buttonHeight) };

        case Style::screen:
        {
            const auto fromSide = scaled (screenButtonGap + ledDiameter);
            const auto capX = labelSide == LabelSide::left ? bounds.getRight() - fromSide - scaled (screenButtonWidth)
                                                           : bounds.getX() + fromSide;
            return { capX, bounds.getY(), scaled (screenButtonWidth), scaled (screenButtonHeight) };
        }

        case Style::ledToggle:
            return {};
    }
    return {};
}

juce::Rectangle<float> RubberButton::getLedBounds() const
{
    using namespace StageMetrics;
    const auto diameter = scaled (ledDiameter);
    const auto bounds = getBoardBox();
    const auto cap = getCapBounds();

    switch (style)
    {
        case Style::key:
            return { cap.getX() - scaled (ledToButtonGap) - diameter, cap.getCentreY() - diameter / halves, diameter, diameter };

        case Style::screen:
        {
            const auto ledX = labelSide == LabelSide::left ? bounds.getRight() - diameter : bounds.getX();
            return { ledX, cap.getCentreY() - diameter / halves, diameter, diameter };
        }

        case Style::ledToggle:
            return { bounds.getX(), bounds.getCentreY() - diameter / halves, diameter, diameter };
    }
    return {};
}

juce::Rectangle<float> RubberButton::getLabelBounds() const
{
    using namespace StageMetrics;
    const auto bounds = getBoardBox();
    switch (style)
    {
        case Style::key:
        {
            const auto top = getCapBounds().getBottom() + scaled (buttonToLabelGap);
            return bounds.withTop (top);
        }

        case Style::screen:
        {
            const auto width = labelWidth();
            const auto labelX = labelSide == LabelSide::left ? bounds.getX() : bounds.getRight() - width;
            return { labelX, bounds.getY(), width, bounds.getHeight() };
        }

        case Style::ledToggle:
            return bounds.withLeft (getLedBounds().getRight() + scaled (toggleGap));
    }
    return {};
}

bool RubberButton::hitTest (int xPosition, int yPosition)
{
    if (style == Style::ledToggle)
        return getBoardBox().contains (static_cast<float> (xPosition), static_cast<float> (yPosition));

    // The cap and its drop edge take clicks; the label and the LED are print on the panel.
    const auto cap = getCapBounds().withHeight (getCapBounds().getHeight() + scaled (StageMetrics::buttonDropEdge));
    return cap.contains (static_cast<float> (xPosition), static_cast<float> (yPosition));
}

void RubberButton::paintButton (juce::Graphics& g, bool, bool isDown)
{
    if (style != Style::ledToggle)
        drawCap (g, getCapBounds(), isDown, scale);

    StageDrawing::drawLed (g, getLedBounds(), ledOn, colourOf (ledColour), scale);

    g.setFont (getLabelFont());
    g.setColour (labelTone == LabelTone::bright ? StagePalette::silkscreenText : StagePalette::silkscreenDimText);

    switch (style)
    {
        case Style::key:
            drawLabelLines (g, labelLines, getLabelBounds(), scaled (StageMetrics::keyLabelLineHeight), juce::Justification::centredTop);
            break;
        case Style::screen:
        case Style::ledToggle:
            drawLabelLines (g, labelLines, getLabelBounds(), getLabelBounds().getHeight(), juce::Justification::centredLeft);
            break;
    }
}

//==============================================================================
StageKnob::StageKnob (const juce::String& labelText, Size knobSize)
    : juce::Slider (juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox),
      label (labelText),
      size (knobSize)
{
    setRange (knobLowest, knobHighest, knobStep);

    const auto fullTurn = juce::MathConstants<float>::twoPi;
    const auto sweep    = juce::degreesToRadians (StageMetrics::knobSweepDegrees);
    setRotaryParameters (fullTurn - sweep, fullTurn + sweep, true);
    setTitle (label);
}

void StageKnob::setScale (float newScale)       { scale = newScale; repaint(); }
StageKnob::Size StageKnob::getSize() const      { return size; }
juce::String StageKnob::getLabel() const        { return label; }
float StageKnob::scaled (float referenceLength) const { return referenceLength * scale; }
float StageKnob::scaled (int referenceLength) const   { return scaled (static_cast<float> (referenceLength)); }

bool StageKnob::angleAround (juce::Point<float> position, double& angle) const
{
    const auto centre = getRingBounds().getCentre();
    if (position.getDistanceFrom (centre) < scaled (StageMetrics::knobDeadZone))
        return false;

    angle = std::atan2 (static_cast<double> (position.x - centre.x), static_cast<double> (centre.y - position.y));   // clockwise from 12 o'clock
    return true;
}

void StageKnob::mouseDown (const juce::MouseEvent& event)
{
    if (! isEnabled())
        return;

    turning = true;
    turnProportion = valueToProportionOfLength (getValue());
    haveTurnAngle = angleAround (event.position, lastTurnAngle);
    if (onDragStart != nullptr)
        onDragStart();
}

void StageKnob::mouseDrag (const juce::MouseEvent& event)
{
    double angle {};
    if (! turning || ! angleAround (event.position, angle))
        return;

    if (! haveTurnAngle)   // pressed in the dead zone: start measuring from here
    {
        lastTurnAngle = angle;
        haveTurnAngle = true;
        return;
    }

    // The turn since the last move, the short way round (crossing 6 o'clock is not a full turn).
    auto delta = angle - lastTurnAngle;
    if (delta > juce::MathConstants<double>::pi)   delta -= juce::MathConstants<double>::twoPi;
    if (delta < -juce::MathConstants<double>::pi)  delta += juce::MathConstants<double>::twoPi;
    lastTurnAngle = angle;

    const auto rotary = getRotaryParameters();
    const auto sweep = static_cast<double> (rotary.endAngleRadians - rotary.startAngleRadians);
    turnProportion = juce::jlimit (double {}, fullTurnProportion, turnProportion + delta / sweep);
    setValue (proportionOfLengthToValue (turnProportion), juce::sendNotificationSync);
}

void StageKnob::mouseUp (const juce::MouseEvent&)
{
    if (! turning)
        return;

    turning = false;
    if (onDragEnd != nullptr)
        onDragEnd();
}

int StageKnob::getPreferredWidth() const
{
    return juce::roundToInt (scaled (geometryFor (size).cellWidth));
}

int StageKnob::getPreferredHeight() const
{
    const auto geometry = geometryFor (size);
    const auto valueLine = showsValue ? geometry.labelGap + StageMetrics::knobValueLineHeight : int {};
    return juce::roundToInt (scaled (geometry.ring + geometry.labelGap + StageMetrics::silkLabelLineHeight + valueLine));
}

void StageKnob::setShowsValue (bool shouldShowValue)
{
    showsValue = shouldShowValue;
    repaint();
}

juce::Rectangle<float> StageKnob::getValueBounds() const
{
    const auto geometry = geometryFor (size);
    const auto top = getRingBounds().getBottom() + scaled (geometry.labelGap + StageMetrics::silkLabelLineHeight + geometry.labelGap);
    return getLocalBounds().toFloat().withTop (top).withHeight (scaled (StageMetrics::knobValueLineHeight));
}

juce::Rectangle<float> StageKnob::getRingBounds() const
{
    const auto ring = scaled (geometryFor (size).ring);
    const auto bounds = getLocalBounds().toFloat();
    return { bounds.getCentreX() - ring / halves, bounds.getY() + knobRingOffsetY, ring, ring };
}

void StageKnob::paint (juce::Graphics& g)
{
    using namespace StageMetrics;
    const auto geometry = geometryFor (size);
    const auto ring     = getRingBounds();
    const auto centre   = ring.getCentre();
    const auto rotary   = getRotaryParameters();

    // Scale ticks: 11 marks from -135 to +135 degrees at the ring's edge.
    g.setColour (StagePalette::knobTick);
    const auto tickWidth = scaled (knobTickWidth);
    const auto tickLength = scaled (geometry.tickLength);
    for (int tickIndex {}; tickIndex <= knobTickIntervals; ++tickIndex)
    {
        const auto proportion = static_cast<float> (tickIndex) / static_cast<float> (knobTickIntervals);
        const auto angle = rotary.startAngleRadians + proportion * (rotary.endAngleRadians - rotary.startAngleRadians);
        juce::Path tick;
        tick.addRectangle (centre.x - tickWidth / halves, ring.getY(), tickWidth, tickLength);
        g.fillPath (tick, juce::AffineTransform::rotation (angle, centre.x, centre.y));
    }

    // Knurled skirt with its drop shadow and dark outline.
    const auto skirtDiameter = scaled (geometry.skirt);
    const auto skirt = juce::Rectangle<float> (skirtDiameter, skirtDiameter).withCentre (centre);
    juce::Path skirtShape;
    skirtShape.addEllipse (skirt);
    juce::DropShadow (juce::Colours::black.withAlpha (knobShadowAlpha), juce::roundToInt (scaled (geometry.shadowBlur)),
                      { int {}, juce::roundToInt (scaled (geometry.shadowOffset)) }).drawForPath (g, skirtShape);

    const auto segment = juce::degreesToRadians (static_cast<float> (geometry.segmentDegrees));
    const auto segmentCount = juce::roundToInt (juce::MathConstants<float>::twoPi / segment);
    for (int segmentIndex {}; segmentIndex < segmentCount; ++segmentIndex)
    {
        const auto segmentStart = segment * static_cast<float> (segmentIndex);
        juce::Path wedge;
        wedge.addPieSegment (skirt, segmentStart, segmentStart + segment, float {});
        g.setColour (segmentIndex % skirtColourCount == int {} ? StagePalette::knobSkirtDark : StagePalette::knobSkirtLight);
        g.fillPath (wedge);
    }
    g.setColour (outlineColour);
    g.drawEllipse (skirt.reduced (scaled (knobOutlineWidth) / halves), scaled (knobOutlineWidth));

    // Cap: radial gradient lit from the upper left, with a thin highlight at the top edge.
    const auto capDiameter = scaled (geometry.cap);
    const auto cap = juce::Rectangle<float> (capDiameter, capDiameter).withCentre (centre);
    const juce::Point<float> lightSpot { cap.getX() + cap.getWidth() * capGradientCentreX, cap.getY() + cap.getHeight() * capGradientCentreY };
    const auto farthestCorner = lightSpot.getDistanceFrom (cap.getBottomRight());
    juce::ColourGradient capFill (StagePalette::knobCapHighlight, lightSpot,
                                  StagePalette::knobCapEdge, lightSpot.translated (farthestCorner, float {}), true);
    capFill.addColour (capGradientMidStop, StagePalette::knobCapMid);
    g.setGradientFill (capFill);
    g.fillEllipse (cap);

    juce::Path highlight;
    const auto highlightArc = juce::degreesToRadians (capHighlightArcDegrees);
    highlight.addCentredArc (centre.x, centre.y, capDiameter / halves - scaled (capHighlightWidth) / halves,
                             capDiameter / halves - scaled (capHighlightWidth) / halves, float {}, -highlightArc, highlightArc, true);
    g.setColour (juce::Colours::white.withAlpha (capHighlightAlpha));
    g.strokePath (highlight, juce::PathStrokeType (scaled (capHighlightWidth)));

    // Pointer: a white line from near the skirt's edge, turned to the value.
    const auto valueAngle = rotary.startAngleRadians
                          + static_cast<float> (valueToProportionOfLength (getValue())) * (rotary.endAngleRadians - rotary.startAngleRadians);
    const auto pointerWidth = scaled (knobPointerWidth);
    juce::Path pointer;
    pointer.addRectangle (centre.x - pointerWidth / halves, skirt.getY() + scaled (geometry.pointerTop), pointerWidth, scaled (geometry.pointerLength));
    g.setColour (StagePalette::knobPointer);
    g.fillPath (pointer, juce::AffineTransform::rotation (valueAngle, centre.x, centre.y));

    // Label under the ring.
    g.setColour (StagePalette::silkscreenText);
    g.setFont (StageFonts::silkscreen (scaled (silkLabelSize), semiBold, silkLabelSpacing));
    const auto labelTop = ring.getBottom() + scaled (geometry.labelGap);
    g.drawText (label.toUpperCase(), getLocalBounds().toFloat().withTop (labelTop), juce::Justification::centredTop, false);

    if (showsValue)
    {
        g.setColour (StagePalette::silkscreenDimText);
        g.setFont (StageFonts::lcd (scaled (knobValueTextSize)));
        g.drawText (juce::String (juce::roundToInt (getValue())), getValueBounds(), juce::Justification::centred, false);
    }
}
