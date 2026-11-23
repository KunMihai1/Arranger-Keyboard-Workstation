/*
  ==============================================================================

    StageScreen.h

    The Stage panel's centred screen (spec 2026-11-23 §5, board StageRevised):
    a bezel holding the LCD (top line, title + tempo, variation / chord / bar)
    and the beat lamps under it.

    Top line: a posted message for 2.5 s, else "STYLE" (the app name while asleep).
    Tempo: wheel (Shift = 5), vertical drag, click to type (Enter applies, Esc
    cancels, leaving the field applies), right-click = the style tempo. While the
    tempo is locked every change is refused and the screen says why.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StageMetrics.h"
#include "StageModels.h"

class StageScreen : public juce::Component, private juce::Timer
{
public:
    struct Content
    {
        juce::String styleName, mode, variation, chord, timeSignature;
        int bar {};
        int beatsPerBar { StageMetrics::defaultBeatsPerBar };
        double elapsedBeats {};
        bool playing {};
        bool asleep {};
    };

    explicit StageScreen (double styleTempo);
    ~StageScreen() override;

    void setScale (float newScale);
    int  getPreferredWidth() const;
    int  getPreferredHeight() const;

    void setContent (const Content& newContent);
    void postMessage (const juce::String& message);

    TempoModel& tempo();
    void setTempoLocked (bool shouldBeLocked);

    /** Shows the open style's tempo (set elsewhere) and its own tempo for right-click; never reported
        back, and left alone while the user is typing a tempo or holding / dragging it. */
    void showTempo (double currentTempo, double styleTempo);

    /** One step from the Tempo -/+ buttons; reports a change, or says the tempo is locked. */
    bool stepTempo (int direction);

    /** Between these, tempo steps show at once but reach onTempoChanged only once, at the end: a hold
        or drag re-scales every track of the style only once, as today's BPM slider did on release. */
    void beginTempoGesture();
    void endTempoGesture();

    /** Called with the new tempo whenever the user changes it on the screen. */
    std::function<void (double)> onTempoChanged;

    /** Replaces the clock (milliseconds) the messages run on; for tests. */
    void setClock (std::function<double()> newClock);

    juce::String getTopLineText() const;
    /** True while the current message takes the whole top row (too long to sit beside the page name). */
    bool isMessageShownAlone() const;
    juce::Rectangle<float> getTempoBounds() const;
    bool isEditingTempo() const;
    juce::TextEditor& getTempoEditor();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent& event) override;
    void mouseExit (const juce::MouseEvent& event) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    void timerCallback() override;

    struct Layout
    {
        juce::Rectangle<float> bezel, lcd, lcdContent, lampRow;
        float topBaseline {}, titleBaseline {}, bottomBaseline {};
    };
    Layout layout() const;

    float scaled (float referenceLength) const;
    float scaled (int referenceLength) const;
    double now() const;
    bool  tempoVisible() const;
    juce::String tempoText() const;
    void  reportTempoChange (bool changed);
    void  openTempoEditor();
    void  closeTempoEditor (bool apply);
    void  paintLamps (juce::Graphics& g, juce::Rectangle<float> lampRow) const;
    juce::String topRightText() const;

    TempoModel tempoModel;
    ScreenMessages messages;
    Content content;
    juce::TextEditor tempoEditor;
    std::function<double()> clock;
    float scale { StageMetrics::referenceScale };
    bool editingTempo {};
    bool mouseOverTempo {};
    bool draggingTempo {};
    bool pressedOnTempo {};
    int  tempoGestures {};          ///< nested holds / drags in progress
    bool tempoReportPending {};     ///< a step happened during them
    float messageOverflow {};   ///< px the current message is wider than the top row (it scrolls)
    int  dragStepsTaken {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StageScreen)
};
