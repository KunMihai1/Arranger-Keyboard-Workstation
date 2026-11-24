#include <JuceHeader.h>
#include "Track.h"
#include "PersistedKeys.h"
#include "TrackEntry.h"

/*  A track slot drawn as the Style page's track strip (board StageRevised openPage=styles, strip 1 at
    (282, 198), 111.25 x 257: number 204, name 296.13 / 222 84 x 20, instrument 294.13 / 248 88 x 22,
    fader 318.13 / 276 40 x 132, value 414, Notes 431 18 high). Picking an instrument keeps the
    track's name (owner, 2026-11-23). */

namespace TrackStripTestValues
{
    const juce::String testName     = "TrackStrip";
    const juce::String testCategory = "Unit";

    const juce::String instrumentKeepsName = "picking an instrument keeps the track name; the button shows the instrument";
    const juce::String captions            = "the instrument button reads Drums / As recorded / - for drums, no instrument, an empty slot";
    const juce::String percussionInfo      = "on a drum track the instrument button reports its message (one line) instead of a pop-up";
    const juce::String stripLayout         = "the strip's parts sit where the board puts them";
    const juce::String longNamesClip       = "a long name is cut with an ellipsis inside its strip, never squashed";
    const juce::String wholeValue          = "the volume reads as a whole number (board: 100, not 100.0)";

    const juce::String bassName      = "Bass";
    const juce::String fingerBass    = "Electric Bass (finger)";
    const juce::String drumsCaption  = "Drums";
    const juce::String recordedCaption = "As recorded";
    const char* const  emptyUtf8     = "\xe2\x80\x94";   // U+2014 em dash
    const juce::String longName      = "A very long track name that will not fit";
    const juce::String loadedVolume  = "100.0";
    const juce::String shownVolume   = "100";
    const juce::String lineBreak     = "\n";

    constexpr int fingerBassProgram = 33;
    constexpr int melodicChannel    = 3;
    constexpr int drumChannel       = 10;
    constexpr float referenceScale  = 1.0f;
    constexpr int stripWidth  = 111;
    constexpr int stripHeight = 257;

    // Board positions inside strip 1 (strip at x 282, y 198; 1 px border at its left).
    constexpr int nameX        = 14;
    constexpr int nameY        = 24;
    constexpr int nameWidth    = 84;
    constexpr int nameHeight   = 20;
    constexpr int instrumentX  = 12;
    constexpr int instrumentY  = 50;
    constexpr int instrumentWidth = 88;
    constexpr int faderX       = 36;
    constexpr int faderY       = 78;
    constexpr int faderWidth   = 40;
    constexpr int faderHeight  = 132;
    constexpr int notesY       = 233;
    constexpr int notesHeight  = 18;
    constexpr int positionTolerance = 1;
    constexpr float noSquash   = 1.0f;
}

class TrackStripTest : public juce::UnitTest
{
public:
    TrackStripTest() : juce::UnitTest (TrackStripTestValues::testName, TrackStripTestValues::testCategory) {}

    static juce::Component* part (Track& track, const juce::String& id)
    {
        return track.findChildWithID (id);
    }

    void expectNear (juce::Rectangle<int> actual, juce::Rectangle<int> expected)
    {
        using namespace TrackStripTestValues;
        expectWithinAbsoluteError (actual.getX(), expected.getX(), positionTolerance);
        expectWithinAbsoluteError (actual.getY(), expected.getY(), positionTolerance);
        expectWithinAbsoluteError (actual.getWidth(), expected.getWidth(), positionTolerance);
        expectWithinAbsoluteError (actual.getHeight(), expected.getHeight(), positionTolerance);
    }

    void runTest() override
    {
        using namespace TrackStripTestValues;

        beginTest (instrumentKeepsName);
        {
            Track track;
            track.setNameLabel (bassName);
            track.setTypeOfTrack (TrackTypeNames::melodic);
            track.setChannel (melodicChannel);
            track.chooseInstrument (fingerBassProgram);
            expectEquals (track.getName(), bassName);
            expectEquals (track.getInstrumentNumber(), fingerBassProgram);
            expectEquals (track.instrumentCaption(), fingerBass);
        }

        beginTest (captions);
        {
            Track drums;
            drums.setNameLabel (drumsCaption);
            drums.setTypeOfTrack (TrackTypeNames::percussion);
            expectEquals (drums.instrumentCaption(), drumsCaption);

            Track melodic;
            melodic.setNameLabel (bassName);
            melodic.setTypeOfTrack (TrackTypeNames::melodic);
            expectEquals (melodic.instrumentCaption(), recordedCaption);

            Track empty;
            expectEquals (empty.instrumentCaption(), juce::String::fromUTF8 (emptyUtf8));
        }

        beginTest (percussionInfo);
        {
            Track track;
            track.setNameLabel (drumsCaption);
            track.setTypeOfTrack (TrackTypeNames::percussion);
            track.setChannel (drumChannel);
            juce::String reported;
            track.onInfo = [&reported] (const juce::String& text) { reported = text; };
            auto* button = dynamic_cast<juce::Button*> (part (track, TrackStripIds::instrument));
            expect (button != nullptr);
            if (button != nullptr)
                button->onClick();
            expect (reported.isNotEmpty());
            expect (! reported.contains (lineBreak));
        }

        beginTest (stripLayout);
        {
            Track track;
            track.setScale (referenceScale);
            track.setSize (stripWidth, stripHeight);
            for (const auto& id : { TrackStripIds::name, TrackStripIds::instrument, TrackStripIds::fader, TrackStripIds::notes })
                expect (part (track, id) != nullptr);
            if (part (track, TrackStripIds::notes) == nullptr)
                return;
            expectNear (part (track, TrackStripIds::name)->getBounds(), { nameX, nameY, nameWidth, nameHeight });
            expectNear (part (track, TrackStripIds::instrument)->getBounds().withHeight (StageMetrics::instrumentHeight),
                        { instrumentX, instrumentY, instrumentWidth, StageMetrics::instrumentHeight });
            expectNear (part (track, TrackStripIds::fader)->getBounds(), { faderX, faderY, faderWidth, faderHeight });
            expectWithinAbsoluteError (part (track, TrackStripIds::notes)->getY(), notesY, positionTolerance);
            expectWithinAbsoluteError (part (track, TrackStripIds::notes)->getHeight(), notesHeight, positionTolerance);
        }

        beginTest (longNamesClip);
        {
            Track track;
            track.setScale (referenceScale);
            track.setSize (stripWidth, stripHeight);
            track.setNameLabel (longName);
            auto* label = dynamic_cast<juce::Label*> (part (track, TrackStripIds::name));
            expect (label != nullptr);
            if (label != nullptr)
            {
                expect (track.getLocalBounds().contains (label->getBounds()));
                expectEquals (label->getMinimumHorizontalScale(), noSquash);
            }
        }

        beginTest (wholeValue);
        {
            Track track;
            track.setVolumeLabel (loadedVolume);
            auto* label = dynamic_cast<juce::Label*> (part (track, TrackStripIds::volume));
            expect (label != nullptr);
            if (label != nullptr)
                expectEquals (label->getText(), shownVolume);
        }
    }
};
static TrackStripTest trackStripTest;
