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
#include "UI/Stage/StageMetrics.h"

namespace
{
    namespace Text
    {
        const juce::String firstInstrument  = "First instrument";
        const juce::String secondInstrument = "Second instrument";
        const juce::String instrument       = "Instrument";
        const juce::String first            = "1st";
        const juce::String second           = "2nd";
        const juce::String sustain          = "Sustain";
        const juce::String tone             = "Tone";
        const juce::String envelope         = "Envelope";
        const juce::String space            = "Space";
        const juce::String modulation       = "Modulation";

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
        const juce::String randomMod  = "Random mod";
    }

    // The instrument choice: first instrument = left hand, second = right hand.
    namespace ChannelItems
    {
        constexpr int first  = 1;
        constexpr int second = 2;
    }

    constexpr double centreValue = 64.0;   // a knob starts (and double-click returns) here, as before

    // Board StageRevised openPage=effects at scale 1, relative to the page content (28, 180).
    namespace Board
    {
        constexpr int columnGap   = 48;   // between the two group columns
        constexpr int rowGap      = 14;   // between the two rows of groups
        constexpr int headingGap  = 6;    // group heading to its knobs
        constexpr int columns     = 2;
        constexpr int cellGaps    = 3;    // four knobs spread across the column: three gaps
    }

    std::array<juce::String, EffectsPageRects::groups> groupNames()
    {
        return { Text::tone, Text::envelope, Text::space, Text::modulation };
    }
}

//==============================================================================
EffectKnobComponent::EffectKnobComponent (const juce::String& name, int ccNumber)
    : knob (name, StageKnob::Size::effects), cc (ccNumber)
{
    knob.setShowsValue (true);
    knob.setValue (centreValue, juce::dontSendNotification);
    knob.setDoubleClickReturnValue (true, centreValue);
    knob.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    knob.onValueChange = [this] { knob.repaint(); };
    knob.onDragEnd = [this]
    {
        if (onValueChanged)
            onValueChanged (cc, juce::roundToInt (knob.getValue()));
    };
    addAndMakeVisible (knob);
}

void EffectKnobComponent::setValue (double newValue, juce::NotificationType notify)
{
    knob.setValue (newValue, notify);
    knob.repaint();
}

double EffectKnobComponent::getValue() const { return knob.getValue(); }

void EffectKnobComponent::setScale (float newScale)
{
    knob.setScale (newScale);
    resized();
}

void EffectKnobComponent::resized()
{
    knob.setBounds (getLocalBounds());
}

//==============================================================================
SustainToggleComponent::SustainToggleComponent()
    : SegButton (Text::sustain, SegButton::Style::seg, true, RubberButton::LedColour::green)
{
    onClick = [this]
    {
        setLedOn (! isLedOn());
        if (onValueChanged)
            onValueChanged (EffectControllers::sustain, isLedOn() ? MidiFacts::highestControllerValue : MidiFacts::lowestControllerValue);
    };
}

bool SustainToggleComponent::isOn() const { return isLedOn(); }
void SustainToggleComponent::setOn (bool shouldBeOn) { setLedOn (shouldBeOn); }

//==============================================================================
EffectsPageRects effectsPageLayout (juce::Rectangle<int> content, float scale)
{
    using namespace StageMetrics;
    auto scaled = [scale] (int boardLength) { return static_cast<float> (boardLength) * scale; };

    const auto area        = content.toFloat();
    const auto columnWidth = (area.getWidth() - scaled (Board::columnGap)) / static_cast<float> (Board::columns);
    const auto cellWidth   = scaled (effectsKnobCellWidth);
    const auto cellHeight  = scaled (effectsKnobRing + effectsKnobLabelGap + silkLabelLineHeight + effectsKnobLabelGap + knobValueLineHeight);
    const auto groupHeight = scaled (headingHeight + Board::headingGap) + cellHeight;
    const auto cellStep    = (columnWidth - cellWidth) / static_cast<float> (Board::cellGaps);

    EffectsPageRects rects;
    for (int group {}; group < EffectsPageRects::groups; ++group)
    {
        const auto column = group % Board::columns;
        const auto row    = group / Board::columns;
        const juce::Point<float> origin { area.getX() + static_cast<float> (column) * (columnWidth + scaled (Board::columnGap)),
                                          area.getY() + static_cast<float> (row) * (groupHeight + scaled (Board::rowGap)) };
        rects.headings[static_cast<size_t> (group)] = juce::Rectangle<float> (origin.x, origin.y, columnWidth, scaled (headingHeight)).toNearestInt();
        for (int knob {}; knob < EffectsPageRects::knobsPerGroup; ++knob)
            rects.knobs[static_cast<size_t> (group * EffectsPageRects::knobsPerGroup + knob)]
                = juce::Rectangle<float> (origin.x + static_cast<float> (knob) * cellStep, origin.y + scaled (headingHeight + Board::headingGap),
                                          cellWidth, cellHeight).toNearestInt();
    }
    return rects;
}

//==============================================================================
SoundEffectContentComponent::SoundEffectContentComponent()
    : instrumentLabel (Text::instrument, SilkLabel::Tone::dim),
      firstButton (Text::first, SegButton::Style::seg, true),
      secondButton (Text::second, SegButton::Style::seg, true),
      brightnessKnob (Text::brightness,  EffectControllers::brightness),
      expressionKnob (Text::expression,  EffectControllers::expression),
      chorusKnob     (Text::chorus,      EffectControllers::chorus),
      resonanceKnob  (Text::resonance,   EffectControllers::resonance),
      attackKnob     (Text::attack,      EffectControllers::attack),
      decayKnob      (Text::decay,       EffectControllers::decay),
      releaseKnob    (Text::release,     EffectControllers::release),
      vibratoKnob    (Text::vibrato,     EffectControllers::vibrato),
      volumeKnob     (Text::volume,      EffectControllers::volume),
      reverbKnob     (Text::reverb,      EffectControllers::reverb),
      delayKnob      (Text::delay,       EffectControllers::delay),
      panKnob        (Text::pan,         EffectControllers::pan),
      distortionKnob (Text::distortion,  EffectControllers::distortion),
      filterTrackKnob(Text::filter,      EffectControllers::filterTrack),
      tremoloKnob    (Text::tremolo,     EffectControllers::tremolo),
      randomModKnob  (Text::randomMod,   EffectControllers::randomMod)
{
    // Today's choice box stays as the state; 1st / 2nd set it, and it reports the channel as before.
    channelSelector.addItem (Text::firstInstrument,  ChannelItems::first);
    channelSelector.addItem (Text::secondInstrument, ChannelItems::second);
    channelSelector.setSelectedId (ChannelItems::first, juce::dontSendNotification);
    channelSelector.onChange = [this]
    {
        refreshInstrumentLeds();
        if (onChannelChanged)
            onChannelChanged (getSelectedChannel());
    };
    addChildComponent (channelSelector);

    firstButton.onClick  = [this] { selectSecondInstrument (false); };
    secondButton.onClick = [this] { selectSecondInstrument (true); };
    refreshInstrumentLeds();

    knobs = { &brightnessKnob, &expressionKnob, &chorusKnob, &resonanceKnob,
              &attackKnob, &decayKnob, &releaseKnob, &vibratoKnob,
              &volumeKnob, &reverbKnob, &delayKnob, &panKnob,
              &distortionKnob, &filterTrackKnob, &tremoloKnob, &randomModKnob };
    for (auto* knob : knobs)
        addAndMakeVisible (knob);
}

int SoundEffectContentComponent::getSelectedChannel() const
{
    return (channelSelector.getSelectedId() == ChannelItems::second) ? HandChannels::rightHand : HandChannels::leftHand;
}

void SoundEffectContentComponent::selectSecondInstrument (bool second)
{
    channelSelector.setSelectedId (second ? ChannelItems::second : ChannelItems::first, juce::sendNotificationSync);
}

void SoundEffectContentComponent::refreshInstrumentLeds()
{
    const bool second = channelSelector.getSelectedId() == ChannelItems::second;
    firstButton.setLedOn (! second);
    secondButton.setLedOn (second);
}

std::vector<juce::Component*> SoundEffectContentComponent::getTitleActions()
{
    return { &instrumentLabel, &firstButton, &secondButton, &sustainDivider, &sustainToggle };
}

void SoundEffectContentComponent::setScale (float newScale)
{
    scale = newScale;
    for (auto* knob : knobs)
        knob->setScale (scale);
    resized();
    repaint();
}

void SoundEffectContentComponent::paint (juce::Graphics& g)
{
    const auto rects = effectsPageLayout (getLocalBounds(), scale);
    const auto names = groupNames();
    for (size_t group {}; group < rects.headings.size(); ++group)
        StageLcd::paintHeading (g, rects.headings[group].toFloat(), scale, names[group], {});
}

void SoundEffectContentComponent::resized()
{
    const auto rects = effectsPageLayout (getLocalBounds(), scale);
    for (size_t knob {}; knob < knobs.size(); ++knob)
        knobs[knob]->setBounds (rects.knobs[knob]);
}
