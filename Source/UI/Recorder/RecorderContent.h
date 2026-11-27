#pragma once
#include <JuceHeader.h>
#include "RecorderModel.h"
#include "UI/Stage/StagePageWidgets.h"

/*  The Recorder page's content (board PageFile): the last recording's LCD and its Play / Stop / Save as MIDI…
    buttons, and the MIDI file's LCD with Play a MIDI file… / Stop. MainComponent gives it the actions and a way
    to read the state; while the page is showing it reads the state four times a second (the time runs while
    recording), and only repaints when something changed. */

enum class RecorderButton { playRecording, stopRecording, saveRecording, playFile, stopFile };

class RecorderContent : public juce::Component, private juce::Timer
{
public:
    RecorderContent();
    ~RecorderContent() override;

    void setScale (float newScale);

    /** Shows `newState`: the LCD texts, which buttons work, Play recording's LED. */
    void setState (const RecorderState& newState);
    const RecorderState& getState() const;

    /** Reads the state through readState and shows it (the poll's tick). */
    void refresh();

    SegButton& getButton (RecorderButton which);

    std::function<RecorderState()> readState;
    std::function<void()> onPlayRecording, onStopRecording, onSaveRecording, onPlayFile, onStopFile;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void layoutButtonRow (juce::Rectangle<int> row, std::initializer_list<SegButton*> buttons);

    RecorderState state;
    float scale { StageMetrics::referenceScale };
    SegButton playRecording, stopRecording, saveRecording, playFile, stopFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RecorderContent)
};
