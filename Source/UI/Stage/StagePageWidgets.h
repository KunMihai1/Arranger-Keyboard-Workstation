/*
  ==============================================================================

    StagePageWidgets.h

    The materials of the redesigned pages (board StageRevised .seg / .inst /
    .fader / .lcd-list / .silk, PageTracks .seg-lit), each drawing itself at the
    Stage scale set with setScale():

      SegButton     flat rubber segment button (.seg), its lit variant (.seg-lit)
                    and the track strips' instrument button (.inst); optional LED
      SilkLabel     silkscreen text (.silk / .silk-dim)
      TitleDivider  the thin vertical rule between groups of title-row buttons
      StageFader    the Style page's channel fader (0-127); dragging moves it by
                    the drag, it never jumps to where it was clicked
      StageLcd      drawing helpers for LCD lists and section headings

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StageMetrics.h"

class SegButton : public juce::Button
{
public:
    enum class Style { seg, lit, instrument };

    SegButton (const juce::String& text, Style style, bool hasLed = false);

    void setLedOn (bool shouldBeOn);
    bool isLedOn() const;
    void setScale (float newScale);

    /** Board height and text size, for the variants the board sizes differently (Notes 18 high,
        "Use in slot" 30 high with 12 px text). */
    void setBoardHeight (int newBoardHeight);
    void setTextSize (float newCssPixelSize);
    void setPaddingSide (int newBoardPadding);   ///< .seg padding 0 10 by default; the strips' Notes use 0 8

    Style getStyle() const;
    int getPreferredWidth() const;    ///< text + padding + border (+ LED and gap)
    int getPreferredHeight() const;   ///< the face; an instrument button also needs its drop edge below
    juce::Rectangle<float> getLedBounds() const;

protected:
    void paintButton (juce::Graphics& g, bool isHighlighted, bool isDown) override;

private:
    juce::Font labelFont() const;
    juce::String shownText() const;
    float scaled (float referenceLength) const;
    float scaled (int referenceLength) const;

    Style style;
    bool hasLed;
    bool ledOn {};
    int boardHeight;
    float textSize { StageMetrics::segLabelSize };
    int paddingSide { StageMetrics::segPaddingSide };
    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SegButton)
};

class SilkLabel : public juce::Component
{
public:
    enum class Tone { bright, dim };

    SilkLabel (const juce::String& text, Tone tone);

    void setText (const juce::String& newText);
    juce::String getText() const;
    void setScale (float newScale);
    /** Board size and letter-spacing for the bigger page titles (14 px, 0.2 em). */
    void setTextStyle (float newCssPixelSize, float newLetterSpacingEm);
    int getPreferredWidth() const;

    void paint (juce::Graphics& g) override;

private:
    juce::Font font() const;

    juce::String text;
    Tone tone;
    float textSize { StageMetrics::silkLabelSize };
    float letterSpacing;
    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SilkLabel)
};

class TitleDivider : public juce::Component
{
public:
    TitleDivider();
    void paint (juce::Graphics& g) override;
};

class StageFader : public juce::Slider
{
public:
    StageFader();

    void setScale (float newScale);

    /** Top of the cap below the fader's top, in board px (the board's capTop). */
    static float capTopFor (double value);

    void paint (juce::Graphics& g) override;

private:
    float scaled (float referenceLength) const;
    float scaled (int referenceLength) const;

    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StageFader)
};

namespace StageLcd
{
    /** The .lcd-list background: LCD green, rounded, with its inset shadow. */
    void paintPanel (juce::Graphics& g, juce::Rectangle<float> area, float scale);

    /** One .lcd-row: text left, an optional mark at the right; selected rows are inverted. */
    void paintRow (juce::Graphics& g, juce::Rectangle<int> row, float scale,
                   const juce::String& leftText, const juce::String& rightText, bool selected);

    juce::Colour textColour (bool selected);
    juce::Font rowFont (float scale);

    /** Where a section heading's rule goes: after the title (+ gap), up to the right edge or the dim text. */
    juce::Rectangle<float> headingRule (juce::Rectangle<float> area, float scale,
                                        const juce::String& title, const juce::String& dimText);

    /** A section heading: silkscreen title, rule, optional dim text at the right. */
    void paintHeading (juce::Graphics& g, juce::Rectangle<float> area, float scale,
                       const juce::String& title, const juce::String& dimText);
}
