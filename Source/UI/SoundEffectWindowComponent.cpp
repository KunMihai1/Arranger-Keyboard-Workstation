/*
  ==============================================================================

    SoundEffectWindowComponent.cpp
    Created: 5 Apr 2026 11:24:18pm
    Author:  Kisuke

  ==============================================================================
*/

#include "SoundEffectWindowComponent.h"
#include "AppColours.h"
#include "MidiHandler.h"

using namespace AppColours;

namespace
{
    namespace Text
    {
        const juce::String windowTitle      = "Sound Effects";
        const juce::String channel          = "Channel";
        const juce::String firstInstrument  = "First instrument";
        const juce::String secondInstrument = "Second instrument";
        const juce::String sustainPedal     = "Sustain Pedal";
        const juce::String on               = "ON";
        const juce::String off              = "OFF";
        const juce::String brightness = "Brightness";
        const juce::String expression = "Expression";
        const juce::String chorus     = "Chorus";
        const juce::String resonance  = "Resonance";
        const juce::String attack     = "Attack";
        const juce::String decay      = "Decay";
        const juce::String release    = "Release";
        const juce::String vibrato    = "Vibrato";
        const juce::String volume     = "Volume";
        const juce::String reverb     = "Reverb";
        const juce::String delay      = "Delay";
        const juce::String pan        = "Pan";
        const juce::String distortion = "Distortion";
        const juce::String filter     = "Filter";
        const juce::String tremolo    = "Tremolo";
        const juce::String randomMod  = "Random Mod";
    }

    // The channel dropdown: first instrument = left hand, second = right hand.
    namespace ChannelItems
    {
        constexpr int first  = 1;
        constexpr int second = 2;
    }

    namespace Knob
    {
        constexpr float  radiusRatio       = 0.38f;
        constexpr float  half              = 0.5f;
        constexpr float  noRotation        = 0.0f;
        constexpr float  trackExtraWidth   = 1.0f;
        constexpr float  glowAlpha         = 0.35f;
        constexpr float  transparent       = 0.0f;
        constexpr float  diameterPerRadius = 2.0f;
        constexpr float  innerRatio        = 0.55f;
        constexpr float  innerLift         = 0.05f;
        constexpr float  outlineThickness  = 1.0f;
        constexpr int    noTextBox         = 0;
        constexpr double lowestValue       = 0.0;     // MIDI controller range
        constexpr double highestValue      = 127.0;
        constexpr double valueStep         = 1.0;
        constexpr double centreValue       = 64.0;
        constexpr float  startTurns        = 1.2f;    // in units of pi
        constexpr float  endTurns          = 2.8f;
        constexpr float  nameFontHeight    = 14.0f;
        constexpr float  valueFontHeight   = 15.0f;
        constexpr int    labelsHeight      = 42;      // value + name under the knob
        constexpr int    inset             = 4;
        constexpr int    valueHeight       = 18;
    }

    namespace Card
    {
        constexpr float inset           = 4.0f;
        constexpr float cornerRadius    = 10.0f;
        constexpr float borderThickness = 1.0f;
        constexpr int   padding         = 8;
    }

    namespace Sustain
    {
        constexpr float nameFontHeight  = 13.0f;
        constexpr float ledLeftOfCentre = 50.0f;
        constexpr float ledAboveCentre  = 6.0f;
        constexpr float glowAlpha       = 0.4f;
        constexpr float glowRadius      = 18.0f;
        constexpr float stateFontHeight = 14.0f;
        constexpr int   stateGap        = 10;
        constexpr int   stateHalfHeight = 10;
        constexpr int   stateWidth      = 50;
        constexpr int   stateHeight     = 20;
        constexpr int   nameHeight      = 22;
    }

    namespace Layout
    {
        constexpr int   margin                 = 16;
        constexpr int   headerH                = 36;
        constexpr int   postHeaderGap          = 14;
        constexpr int   sustainH               = 64;
        constexpr int   sustainGap             = 10;
        constexpr int   rowLabelW              = 22;   // space reserved for the painted row label
        constexpr int   numRows                = 4;
        constexpr int   knobsPerRow            = 4;
        constexpr int   firstRow               = 0;
        constexpr int   bothSides              = 2;
        constexpr int   lineThickness          = 1;
        constexpr int   headerSeparatorY       = 52;
        constexpr float headerFontHeight       = 22.0f;
        constexpr float channelFontHeight      = 13.0f;
        constexpr float depthAlpha             = 0.3f;
        constexpr int   originX                = 0;
        constexpr int   originY                = 0;
        constexpr float rowNameFontHeight      = 16.0f;
        constexpr float accentBarAlpha         = 0.5f;
        constexpr int   accentBarInset         = 8;
        constexpr float accentBarWidth         = 3.0f;
        constexpr float accentBarRadius        = 1.5f;
        constexpr int   rowLabelCentre         = 11;
        constexpr int   centringDivisor        = 2;
        constexpr int   rowNameHalfHeight      = 8;
        constexpr int   rowNameHeight          = 16;
        constexpr int   sustainSeparatorOffset = 2;
        constexpr int   headerLabelWidth       = 180;
        constexpr int   selectorWidth          = 200;
        constexpr int   channelLabelWidth      = 60;
        constexpr int   noInset                = 0;
        constexpr int   selectorInset          = 4;
        constexpr int   sustainMaxWidth        = 320;
        constexpr int   windowX                = 200;
        constexpr int   windowY                = 80;
        constexpr int   windowWidth            = 580;
        constexpr int   windowHeight           = 720;
    }
}

//==============================================================================
//  ArcKnobLookAndFeel
//==============================================================================
void EffectKnobComponent::ArcKnobLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int left, int top, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& slider)
{
    const float radius    = juce::jmin(width, height) * Knob::radiusRatio;
    const float centreX   = (float)left + (float)width  * Knob::half;
    const float centreY   = (float)top + (float)height * Knob::half;
    const float arcWidth  = 3.5f;

    // --- background track ---
    {
        juce::Path track;
        track.addCentredArc(centreX, centreY, radius, radius, Knob::noRotation,
                            rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(knobTrack);
        g.strokePath(track, juce::PathStrokeType(arcWidth + Knob::trackExtraWidth,
                     juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- filled arc ---
    const float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    {
        juce::Path arc;
        arc.addCentredArc(centreX, centreY, radius, radius, Knob::noRotation,
                          rotaryStartAngle, toAngle, true);
        g.setColour(arcColour);
        g.strokePath(arc, juce::PathStrokeType(arcWidth,
                     juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- glow behind the thumb ---
    {
        const float glowRadius = 10.0f;
        const float thumbX = centreX + radius * std::cos(toAngle - juce::MathConstants<float>::halfPi);
        const float thumbY = centreY + radius * std::sin(toAngle - juce::MathConstants<float>::halfPi);

        juce::ColourGradient glow(arcColour.withAlpha(Knob::glowAlpha), thumbX, thumbY,
                                  arcColour.withAlpha(Knob::transparent),
                                  thumbX + glowRadius, thumbY + glowRadius, true);
        g.setGradientFill(glow);
        g.fillEllipse(thumbX - glowRadius, thumbY - glowRadius,
                      glowRadius * Knob::diameterPerRadius, glowRadius * Knob::diameterPerRadius);
    }

    // --- thumb dot ---
    {
        const float thumbRadius = 5.0f;
        const float thumbX = centreX + radius * std::cos(toAngle - juce::MathConstants<float>::halfPi);
        const float thumbY = centreY + radius * std::sin(toAngle - juce::MathConstants<float>::halfPi);

        g.setColour(juce::Colours::white);
        g.fillEllipse(thumbX - thumbRadius, thumbY - thumbRadius,
                      thumbRadius * Knob::diameterPerRadius, thumbRadius * Knob::diameterPerRadius);
    }

    // --- inner circle ---
    {
        const float innerR = radius * Knob::innerRatio;
        g.setColour(cardBg.brighter(Knob::innerLift));
        g.fillEllipse(centreX - innerR, centreY - innerR, innerR * Knob::diameterPerRadius, innerR * Knob::diameterPerRadius);

        g.setColour(knobTrack);
        g.drawEllipse(centreX - innerR, centreY - innerR, innerR * Knob::diameterPerRadius, innerR * Knob::diameterPerRadius, Knob::outlineThickness);
    }
}


//==============================================================================
//  EffectKnobComponent
//==============================================================================
EffectKnobComponent::EffectKnobComponent(const juce::String& name,
                                         int ccNumber,
                                         juce::Colour accentColour)
    : accent(accentColour), cc(ccNumber)
{
    arcLnf.arcColour = accentColour;
    knob.setLookAndFeel(&arcLnf);

    knob.setSliderStyle(juce::Slider::Rotary);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, true, Knob::noTextBox, Knob::noTextBox);
    knob.setRange(Knob::lowestValue, Knob::highestValue, Knob::valueStep);
    knob.setValue(Knob::centreValue, juce::dontSendNotification);
    knob.setDoubleClickReturnValue(true, Knob::centreValue);
    knob.setRotaryParameters(juce::MathConstants<float>::pi * Knob::startTurns,
                             juce::MathConstants<float>::pi * Knob::endTurns,
                             true);  // stopAtEnd prevents wrapping
    knob.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(knob);

    knob.onValueChange = [this]()
    {
        valueLabel.setText(juce::String((int)knob.getValue()), juce::dontSendNotification);
    };

    knob.onDragEnd = [this]()
    {
        if (onValueChanged)
            onValueChanged(cc, (int)knob.getValue());
    };

    nameLabel.setText(name, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, subtitleText);
    nameLabel.setFont(juce::FontOptions(Knob::nameFontHeight));
    addAndMakeVisible(nameLabel);

    valueLabel.setText(juce::String((int)knob.getValue()), juce::dontSendNotification);
    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setColour(juce::Label::textColourId, titleText);
    valueLabel.setFont(juce::FontOptions(Knob::valueFontHeight));
    addAndMakeVisible(valueLabel);
}

void EffectKnobComponent::setValue(double newValue, juce::NotificationType notify)
{
    knob.setValue(newValue, notify);
    valueLabel.setText(juce::String((int)knob.getValue()), juce::dontSendNotification);
}

EffectKnobComponent::~EffectKnobComponent()
{
    knob.setLookAndFeel(nullptr);
}

double EffectKnobComponent::getValue() const { return knob.getValue(); }

void EffectKnobComponent::paint(juce::Graphics& g)
{
    // subtle card background
    auto bounds = getLocalBounds().toFloat().reduced(Card::inset);
    g.setColour(cardBg);
    g.fillRoundedRectangle(bounds, Card::cornerRadius);

    // very faint border
    g.setColour(separator);
    g.drawRoundedRectangle(bounds, Card::cornerRadius, Card::borderThickness);
}

void EffectKnobComponent::resized()
{
    auto area = getLocalBounds().reduced(Card::padding);

    // The knob takes up most of the space
    auto knobArea = area.removeFromTop(area.getHeight() - Knob::labelsHeight);
    knob.setBounds(knobArea.reduced(Knob::inset));

    // Value readout
    valueLabel.setBounds(area.removeFromTop(Knob::valueHeight));

    // Name label at bottom
    nameLabel.setBounds(area);
}


//==============================================================================
//  SustainToggleComponent
//==============================================================================
SustainToggleComponent::SustainToggleComponent()
{
    toggle.setButtonText({});
    toggle.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible(toggle);

    toggle.onClick = [this]()
    {
        repaint();
        if (onValueChanged)
            onValueChanged(EffectControllers::sustain, toggle.getToggleState() ? MidiFacts::highestControllerValue : MidiFacts::lowestControllerValue);
    };

    nameLabel.setText(Text::sustainPedal, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId,subtitleText);
    nameLabel.setFont(juce::FontOptions(Sustain::nameFontHeight));
    addAndMakeVisible(nameLabel);
}

bool SustainToggleComponent::isOn() const { return toggle.getToggleState(); }

void SustainToggleComponent::setOn(bool shouldBeOn)
{
    toggle.setToggleState(shouldBeOn, juce::dontSendNotification);
    repaint();
}

void SustainToggleComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(Card::inset);

    // card background
    g.setColour(cardBg);
    g.fillRoundedRectangle(bounds, Card::cornerRadius);
    g.setColour(separator);
    g.drawRoundedRectangle(bounds, Card::cornerRadius, Card::borderThickness);

    // LED circle
    const bool on = toggle.getToggleState();
    const float ledRadius = 8.0f;
    const float centreX = bounds.getCentreX() - Sustain::ledLeftOfCentre;
    const float centreY = bounds.getCentreY() - Sustain::ledAboveCentre;

    // glow
    if (on)
    {
        juce::ColourGradient glow(sustainOn.withAlpha(Sustain::glowAlpha), centreX, centreY,
                                  sustainOn.withAlpha(Knob::transparent),
                                  centreX + Sustain::glowRadius, centreY + Sustain::glowRadius, true);
        g.setGradientFill(glow);
        g.fillEllipse(centreX - Sustain::glowRadius, centreY - Sustain::glowRadius, Sustain::glowRadius * Knob::diameterPerRadius, Sustain::glowRadius * Knob::diameterPerRadius);
    }

    g.setColour(on ? sustainOn : sustainOff);
    g.fillEllipse(centreX - ledRadius, centreY - ledRadius,
                  ledRadius * Knob::diameterPerRadius, ledRadius * Knob::diameterPerRadius);

    // ON / OFF label next to LED
    g.setColour(titleText);
    g.setFont(juce::FontOptions(Sustain::stateFontHeight));
    g.drawText(on ? Text::on : Text::off,
               (int)(centreX + ledRadius + Sustain::stateGap), (int)(centreY - Sustain::stateHalfHeight), Sustain::stateWidth, Sustain::stateHeight,
               juce::Justification::centredLeft);
}

void SustainToggleComponent::resized()
{
    auto area = getLocalBounds().reduced(Card::padding);

    // The toggle button covers the top area (invisible, just for click handling)
    auto toggleArea = area.removeFromTop(area.getHeight() - Sustain::nameHeight);
    toggle.setBounds(toggleArea);
    toggle.setAlpha(Knob::transparent);   // invisible — we draw our own LED

    nameLabel.setBounds(area);
}


//==============================================================================
//  SoundEffectContentComponent
//==============================================================================
SoundEffectContentComponent::SoundEffectContentComponent()
    : brightnessKnob (Text::brightness,  EffectControllers::brightness, accent1),
      expressionKnob (Text::expression,  EffectControllers::expression, accent1),
      chorusKnob     (Text::chorus,      EffectControllers::chorus, accent1),
      resonanceKnob  (Text::resonance,   EffectControllers::resonance, accent1),
      sustainToggle  (),
      // Line 2 – Envelope
      attackKnob     (Text::attack,      EffectControllers::attack, accent5),
      decayKnob      (Text::decay,       EffectControllers::decay, accent5),
      releaseKnob    (Text::release,     EffectControllers::release, accent5),
      vibratoKnob    (Text::vibrato,     EffectControllers::vibrato, accent5),
      // Line 3 – Space
      volumeKnob     (Text::volume,      EffectControllers::volume, accent3),
      reverbKnob     (Text::reverb,      EffectControllers::reverb, accent3),
      delayKnob      (Text::delay,       EffectControllers::delay, accent3),
      panKnob        (Text::pan,         EffectControllers::pan, accent3),
      // Line 4 – Modulation
      distortionKnob (Text::distortion,  EffectControllers::distortion, accent6),
      filterTrackKnob(Text::filter,      EffectControllers::filterTrack, accent6),
      tremoloKnob    (Text::tremolo,     EffectControllers::tremolo, accent6),
      randomModKnob  (Text::randomMod,   EffectControllers::randomMod, accent6)
{
    // --- header ---
    headerLabel.setText(Text::windowTitle, juce::dontSendNotification);
    headerLabel.setFont(juce::FontOptions(Layout::headerFontHeight));
    headerLabel.setColour(juce::Label::textColourId, titleText);
    headerLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(headerLabel);

    // --- channel selector ---
    channelLabel.setText(Text::channel, juce::dontSendNotification);
    channelLabel.setFont(juce::FontOptions(Layout::channelFontHeight));
    channelLabel.setColour(juce::Label::textColourId, subtitleText);
    channelLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(channelLabel);

    channelSelector.addItem(Text::firstInstrument,  ChannelItems::first);
    channelSelector.addItem(Text::secondInstrument, ChannelItems::second);
    channelSelector.setSelectedId(ChannelItems::first, juce::dontSendNotification);
    channelSelector.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    channelSelector.setColour(juce::ComboBox::backgroundColourId, cardBg);
    channelSelector.setColour(juce::ComboBox::outlineColourId,    separator);
    channelSelector.setColour(juce::ComboBox::textColourId,       titleText);
    channelSelector.setColour(juce::ComboBox::arrowColourId,      accent1);

    channelSelector.onChange = [this]()
    {
        int ch = (channelSelector.getSelectedId() == ChannelItems::second) ? HandChannels::rightHand : HandChannels::leftHand;
        if (onChannelChanged)
            onChannelChanged(ch);
    };
    addAndMakeVisible(channelSelector);

    // --- Line 1 knobs: Tone ---
    addAndMakeVisible(brightnessKnob);
    addAndMakeVisible(expressionKnob);
    addAndMakeVisible(chorusKnob);
    addAndMakeVisible(resonanceKnob);

    // --- Line 2 knobs: Envelope ---
    addAndMakeVisible(attackKnob);
    addAndMakeVisible(decayKnob);
    addAndMakeVisible(releaseKnob);
    addAndMakeVisible(vibratoKnob);

    // --- Line 3 knobs: Space ---
    addAndMakeVisible(volumeKnob);
    addAndMakeVisible(reverbKnob);
    addAndMakeVisible(delayKnob);
    addAndMakeVisible(panKnob);

    // --- Line 4 knobs: Modulation ---
    addAndMakeVisible(distortionKnob);
    addAndMakeVisible(filterTrackKnob);
    addAndMakeVisible(tremoloKnob);
    addAndMakeVisible(randomModKnob);

    // --- sustain ---
    addAndMakeVisible(sustainToggle);
}

int SoundEffectContentComponent::getSelectedChannel() const
{
    return (channelSelector.getSelectedId() == ChannelItems::second) ? HandChannels::rightHand : HandChannels::leftHand;
}

void SoundEffectContentComponent::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(panelBg);

    // Subtle top-to-bottom gradient overlay for depth
    juce::ColourGradient grad(background.withAlpha(Layout::depthAlpha), Layout::originX, Layout::originY,
                              juce::Colours::transparentBlack, Layout::originX, (float)getHeight(), false);
    g.setGradientFill(grad);
    g.fillRect(getLocalBounds());

    // Horizontal separator below header row
    g.setColour(separator);
    g.fillRect(Layout::margin, Layout::headerSeparatorY, getWidth() - Layout::margin * Layout::bothSides, Layout::lineThickness);

    // --- Row section labels ---
    // We compute the same layout metrics used in resized()
    using namespace Layout;

    const int topOfRows  = margin + headerH + postHeaderGap;
    const int bottomLimit = getHeight() - margin - sustainH - sustainGap;
    const int totalRowsH = bottomLimit - topOfRows;
    const int rowH       = totalRowsH / numRows;

    const juce::String rowNames[] = { "TONE", "ENVELOPE", "SPACE", "MODULATION" };
    const juce::Colour rowAccents[] = { accent1, accent5, accent3, accent6 };

    g.setFont(juce::FontOptions(rowNameFontHeight));

    for (int rowIndex {}; rowIndex < numRows; ++rowIndex)
    {
        int rowY = topOfRows + rowIndex * rowH;

        // Subtle separator line above each row (skip the first)
        if (rowIndex > firstRow)
        {
            g.setColour(separator);
            g.fillRect(margin, rowY, getWidth() - margin * bothSides, lineThickness);
        }

        // Vertical accent bar
        g.setColour(rowAccents[rowIndex].withAlpha(accentBarAlpha));
        g.fillRoundedRectangle((float)margin, (float)(rowY + accentBarInset),
                               accentBarWidth, (float)(rowH - accentBarInset * bothSides), accentBarRadius);

        // Rotated row label
        g.setColour(rowLabel);
        g.saveState();
        // Position label centred on the left margin
        float labelCentreX = (float)(margin + rowLabelCentre);
        float labelCentreY = (float)(rowY + rowH / centringDivisor);
        g.addTransform(juce::AffineTransform::rotation(
            -juce::MathConstants<float>::halfPi, labelCentreX, labelCentreY));
        g.drawText(rowNames[rowIndex],
                   (int)(labelCentreX - rowH / centringDivisor), (int)(labelCentreY - rowNameHalfHeight),
                   rowH, rowNameHeight, juce::Justification::centred);
        g.restoreState();
    }

    // Separator above sustain
    int sustainY = bottomLimit + sustainSeparatorOffset;
    g.setColour(separator);
    g.fillRect(margin, sustainY, getWidth() - margin * bothSides, lineThickness);
}

void SoundEffectContentComponent::resized()
{
    using namespace Layout;

    auto area = getLocalBounds().reduced(margin);

    // --- header row: title on left, channel selector on right ---
    auto headerRow = area.removeFromTop(headerH);
    headerLabel.setBounds(headerRow.removeFromLeft(headerLabelWidth));

    auto selectorArea = headerRow.removeFromRight(selectorWidth);
    channelLabel.setBounds(selectorArea.removeFromLeft(channelLabelWidth));
    channelSelector.setBounds(selectorArea.reduced(noInset, selectorInset));

    area.removeFromTop(postHeaderGap);

    // --- Reserve space for sustain at the bottom ---
    auto sustainArea = area.removeFromBottom(sustainH);
    area.removeFromBottom(sustainGap);

    // --- 4 rows of 4 knobs ---
    int rowH = area.getHeight() / numRows;

    auto layoutRow = [&](juce::Rectangle<int> rowArea,
                         EffectKnobComponent& k1, EffectKnobComponent& k2,
                         EffectKnobComponent& k3, EffectKnobComponent& k4)
    {
        // Skip the painted label zone on the left
        rowArea.removeFromLeft(rowLabelW);
        int knobW = rowArea.getWidth() / knobsPerRow;
        k1.setBounds(rowArea.removeFromLeft(knobW));
        k2.setBounds(rowArea.removeFromLeft(knobW));
        k3.setBounds(rowArea.removeFromLeft(knobW));
        k4.setBounds(rowArea);
    };

    layoutRow(area.removeFromTop(rowH), brightnessKnob, expressionKnob,  chorusKnob,    resonanceKnob);
    layoutRow(area.removeFromTop(rowH), attackKnob,     decayKnob,       releaseKnob,   vibratoKnob);
    layoutRow(area.removeFromTop(rowH),  volumeKnob, reverbKnob,  delayKnob,       panKnob);
    layoutRow(area,                     distortionKnob, filterTrackKnob, tremoloKnob,   randomModKnob);

    // --- sustain toggle centred ---
    int sustainWidth = juce::jmin(sustainMaxWidth, sustainArea.getWidth());
    sustainToggle.setBounds(sustainArea.withSizeKeepingCentre(sustainWidth, sustainArea.getHeight()));
}


//==============================================================================
//  SoundEffectWindow  (DocumentWindow)
//==============================================================================
SoundEffectWindow::SoundEffectWindow(juce::PropertiesFile* prop) 
    :propertyFile{ prop }, juce::DocumentWindow(Text::windowTitle,
                           background,
                           DocumentWindow::closeButton)
{
    setUsingNativeTitleBar(false);
    setResizable(false, false);
    setTitleBarTextCentred(true);

    // colour the title bar
    setColour(juce::DocumentWindow::backgroundColourId, background);
    setColour(juce::DocumentWindow::textColourId, titleText);

    content = new SoundEffectContentComponent();
    setContentOwned(content, false);

    setBounds(Layout::windowX, Layout::windowY, Layout::windowWidth, Layout::windowHeight);
    setVisible(true);

    addKeyListener(this);
}

SoundEffectWindow::~SoundEffectWindow()
{
    removeKeyListener(this);
}

bool SoundEffectWindow::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (key == juce::KeyPress::escapeKey)
    {
        closeButtonPressed();
        return true;
    }
    return false;
}

void SoundEffectWindow::closeButtonPressed()
{
    setVisible(false);

    if (onWindowClosed)
        onWindowClosed();
}