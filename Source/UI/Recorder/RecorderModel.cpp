#include "RecorderModel.h"

namespace
{
    constexpr int secondsPerMinute = 60;
    constexpr int longestSeconds   = 5999;   // 99:59
    constexpr int twoDigits        = 2;
    constexpr int oneNote          = 1;
    constexpr int none             = 0;
    constexpr int halves           = 2;
    constexpr double noTime        = 0.0;
    constexpr juce::juce_wchar padDigit = '0';

    const juce::String lengthSeparator = ":";
    const juce::String noteWord        = " note";
    const juce::String notesWord       = " notes";
    const juce::String nothingYet      = "No notes yet";
    const juce::String recOn           = "REC ON";
    const juce::String recOff          = "REC OFF";
    const juce::String noFile          = "NO FILE LOADED";
    const juce::String filePlaying     = "PLAYING";
    const juce::String fileStopped     = "STOPPED";
    const juce::String choosePrompt    = "Choose a .mid file to play it";

    int scaled (int boardPixels, float scale) { return juce::roundToInt (static_cast<float> (boardPixels) * scale); }

    /** A row of `height` at the top of `area`, and `area` left below it. */
    juce::Rectangle<int> takeTop (juce::Rectangle<int>& area, int height) { return area.removeFromTop (height); }
}

bool RecorderState::operator== (const RecorderState& other) const
{
    return recordedSeconds == other.recordedSeconds && notes == other.notes && recording == other.recording
        && playing == other.playing && fileName == other.fileName && filePlaying == other.filePlaying;
}

namespace RecorderModel
{
    juce::String lengthText (double seconds)
    {
        const int whole = juce::jlimit (none, longestSeconds, static_cast<int> (std::floor (juce::jmax (noTime, seconds))));
        return juce::String (whole / secondsPerMinute).paddedLeft (padDigit, twoDigits) + lengthSeparator
             + juce::String (whole % secondsPerMinute).paddedLeft (padDigit, twoDigits);
    }

    juce::String notesText (int notes)
    {
        if (notes <= none)
            return nothingYet;
        return juce::String (notes) + (notes == oneNote ? noteWord : notesWord);
    }

    juce::String recLampText (bool recording) { return recording ? recOn : recOff; }

    juce::String fileStatus (const RecorderState& state)
    {
        if (state.fileName.isEmpty())
            return noFile;
        return state.filePlaying ? filePlaying : fileStopped;
    }

    juce::String fileTitle (const RecorderState& state) { return state.fileName.isEmpty() ? choosePrompt : state.fileName; }

    bool canPlayRecording (const RecorderState& state) { return state.notes > none; }
    bool canStopRecording (const RecorderState& state) { return state.recording || state.playing; }
    bool canSaveRecording (const RecorderState& state) { return state.notes > none && ! state.recording; }
    bool canStopFile (const RecorderState& state)      { return state.filePlaying; }
}

RecorderRects recorderLayout (juce::Rectangle<int> content, float scale)
{
    using namespace RecorderMetrics;
    RecorderRects rects;

    const int gap         = scaled (columnGap, scale);
    const int columnWidth = (content.getWidth() - gap) / columnCount;
    auto left  = content.withWidth (columnWidth);
    auto right = content.withLeft (content.getRight() - columnWidth);

    const int heading   = scaled (headingHeight, scale);
    const int afterHead = scaled (headingGap, scale);
    const int lcdGap    = scaled (lcdButtonsGap, scale);
    const int buttons   = scaled (buttonHeight, scale);

    rects.recordingHeading = takeTop (left, heading);
    left.removeFromTop (afterHead);
    rects.recordingLcd = takeTop (left, scaled (recordingLcdHeight, scale));
    left.removeFromTop (lcdGap);
    rects.recordingButtons = takeTop (left, buttons);
    left.removeFromTop (scaled (hintGap, scale));
    rects.hint = takeTop (left, scaled (hintHeight, scale));

    rects.fileHeading = takeTop (right, heading);
    right.removeFromTop (afterHead);
    rects.fileLcd = takeTop (right, scaled (fileLcdHeight, scale));
    right.removeFromTop (lcdGap);
    rects.fileButtons = takeTop (right, buttons);

    const int padX      = scaled (lcdPaddingX, scale);
    const int padTop    = scaled (lcdPaddingTop, scale);
    const int padBottom = scaled (lcdPaddingBottom, scale);
    const int smallLine = scaled (smallLineHeight, scale);

    // Recording LCD: label + lamp on the top line, notes on the bottom line, the big time centred between them.
    auto recordingInner = rects.recordingLcd.reduced (padX, none).withTrimmedTop (padTop).withTrimmedBottom (padBottom);
    auto labelRow = takeTop (recordingInner, smallLine);
    rects.recordingLabel = labelRow.withWidth (labelRow.getWidth() / halves);
    rects.recLamp = labelRow.withLeft (rects.recordingLabel.getRight());
    rects.notes = recordingInner.removeFromBottom (smallLine);
    rects.time = recordingInner.withSizeKeepingCentre (recordingInner.getWidth(),
                                                       juce::jmin (recordingInner.getHeight(), scaled (timeHeight, scale)));

    // File LCD: status on top, the subtitle at the bottom, the file's name centred between them.
    auto fileInner = rects.fileLcd.reduced (padX, none).withTrimmedTop (padTop).withTrimmedBottom (padBottom);
    rects.fileStatus = takeTop (fileInner, smallLine);
    rects.fileSubtitle = fileInner.removeFromBottom (smallLine);
    rects.fileTitle = fileInner.withSizeKeepingCentre (fileInner.getWidth(),
                                                       juce::jmin (fileInner.getHeight(), scaled (fileTitleHeight, scale)));
    return rects;
}
