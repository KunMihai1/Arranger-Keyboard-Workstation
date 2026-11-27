#include <JuceHeader.h>
#include "UI/Recorder/RecorderModel.h"
#include "UI/Recorder/RecorderContent.h"

/*  The Recorder page (board PageFile, spec part 2 P.9). Today the File page only held the old header buttons and
    showed nothing of the recording; the board shows its length, notes and REC state, and the MIDI file's state. */

namespace RecorderModelTestValues
{
    const juce::String testName     = "RecorderModel";
    const juce::String testCategory = "Unit";

    const juce::String lengthIsMinutesSeconds = "the length reads mm:ss, from 00:00 up to 99:59";
    const juce::String notesCounted           = "the notes line counts notes, singular for one; none reads true before a take and after an empty one";
    const juce::String lampFollowsRecording   = "the lamp reads REC ON while recording, REC OFF otherwise";
    const juce::String fileLines              = "the file LCD: no file, playing, stopped; the file's name or the prompt";
    const juce::String buttonsWork            = "Play needs notes; Stop needs something running; Save needs a finished recording";
    const juce::String layoutMatchesBoard     = "two equal columns 40 apart: heading, LCD, buttons; the hint under the recording";
    const juce::String layoutScales           = "the layout is the board times the scale";
    const juce::String contentTestName        = "RecorderContent";
    const juce::String buttonsFollowState     = "the page's buttons work only when the model says, Play recording's LED lit while it plays";
    const juce::String clicksReachActions     = "each button runs its action";
    const juce::String refreshReadsState      = "a poll tick shows the state MainComponent reports";

    const juce::String zeroLength      = "00:00";
    const juce::String boardLength     = "00:42";
    const juce::String twelveMinutes   = "12:05";
    const juce::String longestLength   = "99:59";
    const juce::String boardNotes      = "312 notes";
    const juce::String oneNote         = "1 note";
    const juce::String noNotes         = "No notes yet";
    const juce::String recOn           = "REC ON";
    const juce::String recOff          = "REC OFF";
    const juce::String noFile          = "NO FILE LOADED";
    const juce::String filePlaying     = "PLAYING";
    const juce::String fileStopped     = "STOPPED";
    const juce::String choosePrompt    = "Choose a .mid file to play it";
    const juce::String songFile        = "Moonlight.mid";

    constexpr double noTime            = 0.0;
    constexpr double boardSeconds      = 42.6;
    constexpr double twelveMinutesFive = 725.0;
    constexpr double hours             = 7200.0;
    constexpr double negativeTime      = -3.0;
    constexpr int    boardNoteCount    = 312;
    constexpr int    singleNote        = 1;
    constexpr int    nothing           = 0;
    constexpr int    contentWidth      = 1144;   // the page content at 1200 × 645 (28 px sides)
    constexpr int    contentHeight     = 500;
    constexpr float  boardScale        = 1.0f;
    constexpr float  doubleScale       = 2.0f;
    constexpr int    doubled           = 2;
    constexpr int    onePixel          = 1;
}

class RecorderModelTest : public juce::UnitTest
{
public:
    RecorderModelTest() : juce::UnitTest (RecorderModelTestValues::testName, RecorderModelTestValues::testCategory) {}

    void runTest() override
    {
        using namespace RecorderModelTestValues;
        using namespace RecorderModel;

        beginTest (lengthIsMinutesSeconds);
        expectEquals (lengthText (noTime), zeroLength);
        expectEquals (lengthText (boardSeconds), boardLength);
        expectEquals (lengthText (twelveMinutesFive), twelveMinutes);
        expectEquals (lengthText (hours), longestLength);
        expectEquals (lengthText (negativeTime), zeroLength);

        beginTest (notesCounted);
        expectEquals (notesText (boardNoteCount), boardNotes);
        expectEquals (notesText (singleNote), oneNote);
        expectEquals (notesText (nothing), noNotes);

        beginTest (lampFollowsRecording);
        expectEquals (recLampText (true), recOn);
        expectEquals (recLampText (false), recOff);

        beginTest (fileLines);
        {
            RecorderState state;
            expectEquals (fileStatus (state), noFile);
            expectEquals (fileTitle (state), choosePrompt);
            state.fileName = songFile;
            state.filePlaying = true;
            expectEquals (fileStatus (state), filePlaying);
            expectEquals (fileTitle (state), songFile);
            state.filePlaying = false;
            expectEquals (fileStatus (state), fileStopped);
        }

        beginTest (buttonsWork);
        {
            RecorderState state;
            expect (! canPlayRecording (state) && ! canStopRecording (state) && ! canSaveRecording (state) && ! canStopFile (state));
            state.recording = true;
            state.notes = boardNoteCount;
            expect (canStopRecording (state));
            expect (! canSaveRecording (state));   // finish the recording first
            state.recording = false;
            expect (canPlayRecording (state) && canSaveRecording (state) && ! canStopRecording (state));
            state.playing = true;
            expect (canStopRecording (state));
            state.filePlaying = true;
            expect (canStopFile (state));
        }

        beginTest (layoutMatchesBoard);
        {
            using namespace RecorderMetrics;
            const juce::Rectangle<int> content (contentWidth, contentHeight);
            const auto rects = recorderLayout (content, boardScale);
            expectEquals (rects.recordingLcd.getWidth(), rects.fileLcd.getWidth());
            expectEquals (rects.fileLcd.getX() - rects.recordingLcd.getRight(), columnGap);
            expectEquals (rects.recordingLcd.getX(), content.getX());
            expectEquals (rects.fileLcd.getRight(), content.getRight());
            expectEquals (rects.recordingLcd.getHeight(), recordingLcdHeight);
            expectEquals (rects.fileLcd.getHeight(), fileLcdHeight);
            expectEquals (rects.recordingLcd.getY(), rects.recordingHeading.getBottom() + headingGap);
            expectEquals (rects.recordingButtons.getY(), rects.recordingLcd.getBottom() + lcdButtonsGap);
            expectEquals (rects.fileButtons.getY(), rects.fileLcd.getBottom() + lcdButtonsGap);
            expectEquals (rects.recordingButtons.getHeight(), buttonHeight);
            expectEquals (rects.hint.getY(), rects.recordingButtons.getBottom() + hintGap);
            for (auto inner : { rects.recordingLabel, rects.recLamp, rects.time, rects.notes })
                expect (rects.recordingLcd.contains (inner));
            for (auto inner : { rects.fileStatus, rects.fileTitle, rects.fileSubtitle })
                expect (rects.fileLcd.contains (inner));
            expect (rects.recordingLabel.getBottom() <= rects.time.getY() && rects.time.getBottom() <= rects.notes.getY());
            expect (rects.fileStatus.getBottom() <= rects.fileTitle.getY() && rects.fileTitle.getBottom() <= rects.fileSubtitle.getY());
        }

        beginTest (layoutScales);
        {
            const auto atOne = recorderLayout ({ contentWidth, contentHeight }, boardScale);
            const auto atTwo = recorderLayout ({ contentWidth * doubled, contentHeight * doubled }, doubleScale);
            expectWithinAbsoluteError (atTwo.recordingLcd.getHeight(), atOne.recordingLcd.getHeight() * doubled, onePixel);
            expectWithinAbsoluteError (atTwo.fileLcd.getX(), atOne.fileLcd.getX() * doubled, onePixel);
            expectWithinAbsoluteError (atTwo.hint.getY(), atOne.hint.getY() * doubled, onePixel);
        }
    }
};
static RecorderModelTest recorderModelTest;

class RecorderContentTest : public juce::UnitTest
{
public:
    RecorderContentTest() : juce::UnitTest (RecorderModelTestValues::contentTestName, RecorderModelTestValues::testCategory) {}

    void runTest() override
    {
        using namespace RecorderModelTestValues;

        beginTest (buttonsFollowState);
        {
            RecorderContent content;
            content.setState ({});
            expect (! content.getButton (RecorderButton::playRecording).isEnabled());
            expect (! content.getButton (RecorderButton::stopRecording).isEnabled());
            expect (! content.getButton (RecorderButton::saveRecording).isEnabled());
            expect (content.getButton (RecorderButton::playFile).isEnabled());   // choosing a file always works
            expect (! content.getButton (RecorderButton::stopFile).isEnabled());

            RecorderState playing;
            playing.notes = boardNoteCount;
            playing.playing = true;
            playing.filePlaying = true;
            content.setState (playing);
            expect (content.getButton (RecorderButton::playRecording).isEnabled());
            expect (content.getButton (RecorderButton::stopRecording).isEnabled());
            expect (content.getButton (RecorderButton::saveRecording).isEnabled());
            expect (content.getButton (RecorderButton::stopFile).isEnabled());
            expect (content.getButton (RecorderButton::playRecording).isLedOn());

            playing.playing = false;
            content.setState (playing);
            expect (! content.getButton (RecorderButton::playRecording).isLedOn());
        }

        beginTest (clicksReachActions);
        {
            RecorderContent content;
            std::vector<RecorderButton> ran;
            content.onPlayRecording = [&ran] { ran.push_back (RecorderButton::playRecording); };
            content.onStopRecording = [&ran] { ran.push_back (RecorderButton::stopRecording); };
            content.onSaveRecording = [&ran] { ran.push_back (RecorderButton::saveRecording); };
            content.onPlayFile      = [&ran] { ran.push_back (RecorderButton::playFile); };
            content.onStopFile      = [&ran] { ran.push_back (RecorderButton::stopFile); };
            const std::vector<RecorderButton> all { RecorderButton::playRecording, RecorderButton::stopRecording, RecorderButton::saveRecording,
                                                    RecorderButton::playFile, RecorderButton::stopFile };
            for (auto which : all)
            {
                auto& button = content.getButton (which);
                expect (button.onClick != nullptr);
                if (button.onClick != nullptr)
                    button.onClick();
            }
            expect (ran == all);
        }

        beginTest (refreshReadsState);
        {
            RecorderContent content;
            RecorderState reported;
            reported.notes = boardNoteCount;
            reported.fileName = songFile;
            content.readState = [&reported] { return reported; };
            content.refresh();
            expect (content.getState() == reported);
            expect (content.getButton (RecorderButton::playRecording).isEnabled());
        }
    }
};
static RecorderContentTest recorderContentTest;
