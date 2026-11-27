#include "RecorderContent.h"
#include "Common/AppColours.h"
#include "UI/Stage/StageFonts.h"

namespace
{
    namespace RecorderText
    {
        const juce::String playRecording = "Play recording";
        const juce::String stop          = "Stop";
        const char* const saveAsMidiUtf8 = "Save as MIDI\xe2\x80\xa6";        // "Save as MIDI…"
        const char* const playFileUtf8   = "Play a MIDI file\xe2\x80\xa6";    // "Play a MIDI file…"
        const juce::String recordingHeading = "Your last recording";
        const juce::String fileHeading      = "MIDI file";
        const juce::String lastRecording    = "LAST RECORDING";
        const char* const lampUtf8          = "\xe2\x97\x8f ";      // a filled circle and a space
        const juce::String fileSubtitle     = "through the current sounds";
        const juce::String hint             = "Press REC on the panel to start and stop recording.";
    }

    constexpr int   pollHz        = 4;       // the time runs while recording: four reads a second
    constexpr int   paddingSide   = 12;      // the board's .seg: padding 0 12px
    constexpr float detailTextSize = 13.0f;  // the board's "312 events" / "through the current sounds"
    constexpr bool  mediumFace    = false;   // silkscreen Medium (.silk-dim)
    constexpr float buttonTextSize = 11.0f;  // PageFile's .seg: 26 high, 11 px text (the shared default is 22 / 10)
    constexpr float hintTextSize  = 11.0f;   // PageFile's .silk-dim: 11 px, letter-spacing .06em, as written
    constexpr float hintSpacing   = 0.06f;
}

RecorderContent::RecorderContent()
    : playRecording (RecorderText::playRecording, SegButton::Style::lit, true),
      stopRecording (RecorderText::stop, SegButton::Style::seg),
      saveRecording (juce::String::fromUTF8 (RecorderText::saveAsMidiUtf8), SegButton::Style::seg),
      playFile (juce::String::fromUTF8 (RecorderText::playFileUtf8), SegButton::Style::lit),
      stopFile (RecorderText::stop, SegButton::Style::seg)
{
    const auto run = [] (const std::function<void()>& action) { if (action != nullptr) action(); };
    playRecording.onClick = [this, run] { run (onPlayRecording); };
    stopRecording.onClick = [this, run] { run (onStopRecording); };
    saveRecording.onClick = [this, run] { run (onSaveRecording); };
    playFile.onClick      = [this, run] { run (onPlayFile); };
    stopFile.onClick      = [this, run] { run (onStopFile); };
    for (auto* button : { &playRecording, &stopRecording, &saveRecording, &playFile, &stopFile })
    {
        button->setPaddingSide (paddingSide);
        button->setBoardHeight (RecorderMetrics::buttonHeight);
        button->setTextSize (buttonTextSize);
        button->setMouseClickGrabsKeyboardFocus (false);   // the PC keyboard keeps playing after a press
        addAndMakeVisible (button);
    }
    setState (state);
    startTimerHz (pollHz);
}

RecorderContent::~RecorderContent() = default;

void RecorderContent::setScale (float newScale)
{
    scale = newScale;
    for (auto* button : { &playRecording, &stopRecording, &saveRecording, &playFile, &stopFile })
        button->setScale (scale);
    resized();
    repaint();
}

void RecorderContent::setState (const RecorderState& newState)
{
    state = newState;
    playRecording.setEnabled (RecorderModel::canPlayRecording (state));
    playRecording.setLedOn (state.playing);
    stopRecording.setEnabled (RecorderModel::canStopRecording (state));
    saveRecording.setEnabled (RecorderModel::canSaveRecording (state));
    stopFile.setEnabled (RecorderModel::canStopFile (state));
    repaint();
}

const RecorderState& RecorderContent::getState() const { return state; }

void RecorderContent::refresh()
{
    if (readState == nullptr)
        return;
    const auto reported = readState();
    if (reported != state)
        setState (reported);
}

void RecorderContent::timerCallback()
{
    if (isShowing())   // the page is closed: nothing to read
        refresh();
}

SegButton& RecorderContent::getButton (RecorderButton which)
{
    switch (which)
    {
        case RecorderButton::playRecording: return playRecording;
        case RecorderButton::stopRecording: return stopRecording;
        case RecorderButton::saveRecording: return saveRecording;
        case RecorderButton::playFile:      return playFile;
        case RecorderButton::stopFile:      break;
    }
    return stopFile;
}

void RecorderContent::paint (juce::Graphics& g)
{
    using namespace RecorderMetrics;
    const auto rects = recorderLayout (getLocalBounds(), scale);

    StageLcd::paintHeading (g, rects.recordingHeading.toFloat(), scale, RecorderText::recordingHeading, {});
    StageLcd::paintHeading (g, rects.fileHeading.toFloat(), scale, RecorderText::fileHeading, {});
    StageLcd::paintPanel (g, rects.recordingLcd.toFloat(), scale);
    StageLcd::paintPanel (g, rects.fileLcd.toFloat(), scale);

    g.setColour (StagePalette::lcdInk);
    g.setFont (StageFonts::lcd (smallTextSize * scale));
    g.drawText (RecorderText::lastRecording, rects.recordingLabel, juce::Justification::centredLeft, false);
    g.drawText (juce::String::fromUTF8 (RecorderText::lampUtf8) + RecorderModel::recLampText (state.recording), rects.recLamp,
                juce::Justification::centredRight, false);
    g.drawText (RecorderModel::fileStatus (state), rects.fileStatus, juce::Justification::centredLeft, false);

    g.setFont (StageFonts::lcd (timeTextSize * scale));
    g.drawText (RecorderModel::lengthText (state.recordedSeconds), rects.time, juce::Justification::centredLeft, false);

    g.setFont (StageFonts::lcd (detailTextSize * scale));
    g.drawText (RecorderModel::notesText (state.notes), rects.notes, juce::Justification::centredLeft, false);
    g.drawText (RecorderText::fileSubtitle, rects.fileSubtitle, juce::Justification::centredLeft, false);

    g.setFont (StageFonts::lcd (fileTitleTextSize * scale));
    g.drawText (RecorderModel::fileTitle (state), rects.fileTitle, juce::Justification::centredLeft, true);

    g.setColour (StagePalette::silkscreenDimText);
    g.setFont (StageFonts::silkscreen (hintTextSize * scale, mediumFace, hintSpacing));
    g.drawText (RecorderText::hint, rects.hint, juce::Justification::centredLeft, false);
}

void RecorderContent::resized()
{
    const auto rects = recorderLayout (getLocalBounds(), scale);
    layoutButtonRow (rects.recordingButtons, { &playRecording, &stopRecording, &saveRecording });
    layoutButtonRow (rects.fileButtons, { &playFile, &stopFile });
}

void RecorderContent::layoutButtonRow (juce::Rectangle<int> row, std::initializer_list<SegButton*> buttons)
{
    auto left = static_cast<float> (row.getX());   // exact widths: a row of buttons places them without drift
    for (auto* button : buttons)
    {
        button->setBounds (juce::Rectangle<float> (left, static_cast<float> (row.getY()), button->getExactWidth(),
                                                   static_cast<float> (row.getHeight())).toNearestInt());
        left += button->getExactWidth() + static_cast<float> (RecorderMetrics::buttonGap) * scale;
    }
}
