#include "MainComponent.h"

// The small widgets MainComponent draws with: rotary slider, knob and button looks, panels
// (split from MainComponent.cpp, ADR-003).

namespace
{
    constexpr float dragSensitivity      = 0.005f;   // value range per pixel dragged
    constexpr float knobStartTurns       = 1.25f;    // in units of pi
    constexpr float knobEndTurns         = 2.75f;
    constexpr float proportionStart      = 0.0f;
    constexpr float proportionEnd        = 1.0f;
    constexpr float half                 = 0.5f;
    constexpr float fallbackKnobInset    = 4.0f;
    constexpr float diameterPerRadius    = 2.0f;
    constexpr float pointerThickness     = 3.0f;
    constexpr float buttonFontHeight     = 40.0f;
    constexpr int   panelBorderThickness = 3;
    constexpr float headerGlowAlpha      = 0.18f;
    constexpr float gradientLeft         = 0.0f;
    constexpr float gradientTop          = 0.0f;
    constexpr int   stripLeft            = 0;
    constexpr int   stripTop             = 0;
    constexpr int   accentStripHeight    = 3;
}

SmoothRotarySlider::SmoothRotarySlider()
{
    setSliderStyle(juce::Slider::Rotary);
}

void SmoothRotarySlider::mouseDown(const juce::MouseEvent& event)
{
    dragStartValue = getValue(); 
    dragStartPos = event.position; 
    juce::Slider::mouseDown(event);
}

void SmoothRotarySlider::mouseDrag(const juce::MouseEvent& event)
{
    float dx = event.position.x - dragStartPos.x;
    float dy = dragStartPos.y - event.position.y;

    float sensitivity = dragSensitivity;

    float delta = dx + dy;

    float newValue = dragStartValue + delta * sensitivity * (getMaximum() - getMinimum());
    setValue(newValue, juce::dontSendNotification);
}

KnobLookAndFeel::KnobLookAndFeel()
{
    rawKnobImage = juce::ImageCache::getFromMemory(BinaryData::Knob_png, BinaryData::Knob_pngSize);
    rotaryStartAngle = juce::MathConstants<float>::pi * knobStartTurns;
    rotaryEndAngle = juce::MathConstants<float>::pi * knobEndTurns; 
}

void KnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int left, int top, int width, int height, float sliderPosProportional, float rotaryStart, float rotaryEnd, juce::Slider& slider)
{
    float startAngle = rotaryStartAngle; 
    
    
    float endAngle = rotaryEndAngle;

    sliderPosProportional = juce::jlimit(proportionStart, proportionEnd, sliderPosProportional);

    float angle = startAngle + sliderPosProportional * (endAngle - startAngle);

    const float cx = left + width * half;
    const float cy = top + height * half; 
    const int knobSize = juce::jmin(width, height);

    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    if (rawKnobImage.isValid())
    {
        juce::Image scaledKnob = rawKnobImage.rescaled(knobSize, knobSize);
        juce::AffineTransform transform = juce::AffineTransform::translation(-scaledKnob.getWidth() * half, -scaledKnob.getHeight() * half).rotated(angle).translated(cx, cy);
        g.drawImageTransformed(scaledKnob, transform);
    }
    else
    {
        float radius = knobSize * half - fallbackKnobInset;
        g.setColour(juce::Colours::darkgrey); 
        
        g.fillEllipse(cx - radius, cy - radius, radius * diameterPerRadius, radius * diameterPerRadius);

        g.setColour(juce::Colours::orange);

        juce::Point<float> lineStart(cx, cy); 
        juce::Point<float> lineEnd(cx + radius * std::cos(angle), cy + radius * std::sin(angle)); 
        g.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, pointerThickness);
    }
}

juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
{
    return juce::Font(buttonFontHeight, juce::Font::bold);
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown)
{
    juce::ignoreUnused(button, isMouseOverButton, isButtonDown); g.fillAll(backgroundColour);
}

void MainComponent::Panel::paint(juce::Graphics& g)
{
    g.fillAll(AppColours::settingsPanelFill); // Background of the panel
    g.setColour(AppColours::settingsPanelBorder);
    g.drawRect(getLocalBounds(), panelBorderThickness);
}

void MainComponent::headerPanel::paint(juce::Graphics& g)
{
    // Base dark fill
    g.fillAll(AppColours::panelBg);

    // Subtle purple gradient overlay (fades from top)
    juce::ColourGradient overlay(AppColours::accent2.withAlpha(headerGlowAlpha), gradientLeft, gradientTop,
                                 juce::Colours::transparentBlack, gradientLeft, (float)getHeight(), false);
    g.setGradientFill(overlay);
    g.fillAll();

    // Top accent strip
    g.setColour(AppColours::accent2);
    g.fillRect(stripLeft, stripTop, getWidth(), accentStripHeight);
}
