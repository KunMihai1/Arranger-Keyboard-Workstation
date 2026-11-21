#include <JuceHeader.h>
#include "SFZlibrary.h"
#include "IOHelper.h"

/*
    SFZLibraryManager keeps the user's SFZ instrument library and the per-style "instrument number ->
    SFZ" assignments, and saves them to SFZLibrary.json. It had no tests; a mistake here silently plays
    the wrong instrument or loses the user's assignments. All files live in the temp folder.
*/
namespace SfzLibraryTestValues
{
    const juce::String testName     = "SFZLibraryManager";
    const juce::String testCategory = "Unit";

    const juce::String sfzExtension      = ".sfz";
    const juce::String jsonExtension     = ".json";
    const juce::String sfzFileContents   = "<region> sample=piano.wav";
    const juce::String missingSfzName    = "does-not-exist.sfz";
    const juce::String popStyle          = "Pop";
    const juce::String jazzStyle         = "Jazz";
    const juce::String emptyStyle        = "";
    const juce::String unknownEntryId    = "no-such-entry";
    const juce::String missingJsonName   = "no-such-sfz-library.json";
    const juce::String corruptJson       = "{ this is not json";
    const juce::String missingStyle      = "No such style";

    constexpr int pianoInstrument        = 0;     // General MIDI 1: Acoustic Grand Piano
    constexpr int bassInstrument         = 33;    // General MIDI 34: Electric Bass (finger)
    constexpr int lastGmInstrument       = 127;   // General MIDI 128: Gunshot
    constexpr int belowGmRange           = -1;
    constexpr int aboveGmRange           = 128;

    const juce::String firstGmName       = "Acoustic Grand Piano";
    const juce::String lastGmName        = "Gunshot";
    const juce::String outOfRangePrefix  = "Instrument ";

    constexpr size_t oneLibraryEntry     = 1;
    constexpr int    generalMidiInstrumentCount = 128;
}

namespace SfzLibraryTestText
{
    const juce::String addTitle       = "addFile: a missing file is rejected, the same file is added once";
    const juce::String assignTitle    = "assignToStyleInstrument: rejects an empty style, out-of-range numbers and unknown entries";
    const juce::String removeTitle    = "removeEntry: drops the entry and every assignment to it";
    const juce::String clearTitle     = "clearStyleInstrumentAssignment: removes one assignment, and the style once empty";
    const juce::String importTitle    = "importMappingsFromStyle: copies onto the target, overwriting clashes";
    const juce::String roundTripTitle = "save then load restores the library and the assignments exactly";
    const juce::String gmNamesTitle   = "getGMInstrumentName: names 0 and 127, falls back outside the range";
    const juce::String badFileTitle   = "load: a missing or corrupt file gives an empty library, not stale data or a crash";
    const juce::String importNoOpTitle = "importMappingsFromStyle: a missing source or the same style changes nothing";

    const juce::String missingFileAccepted   = "a missing file must not be added";
    const juce::String existingFileRejected  = "an existing SFZ file should get an id";
    const juce::String duplicateAddedTwice   = "adding the same file twice must return the same id, not a second entry";
    const juce::String unexpectedEntryCount  = "unexpected number of library entries";
    const juce::String validAssignmentLost   = "a valid assignment did not resolve to its SFZ file";
    const juce::String invalidAssignmentKept = "an invalid assignment was stored";
    const juce::String removedEntryStillUsed = "an assignment still points at a removed entry";
    const juce::String emptyStyleKept        = "a style with no assignments left should be dropped";
    const juce::String otherAssignmentLost   = "clearing one assignment removed another";
    const juce::String importMissing         = "an imported assignment is missing";
    const juce::String importDidNotOverwrite = "an imported assignment should overwrite the target's";
    const juce::String targetOnlyLost        = "the target's own other assignments must survive an import";
    const juce::String roundTripDiffers      = "a value changed between save and load";
    const juce::String gmNameWrong           = "wrong General MIDI name";
    const juce::String staleDataAfterBadLoad = "loading a missing or corrupt file kept old data";
    const juce::String noOpImportChanged     = "an import that should do nothing changed the assignments";
}

class SfzLibraryTest : public juce::UnitTest
{
public:
    SfzLibraryTest() : juce::UnitTest (SfzLibraryTestValues::testName, SfzLibraryTestValues::testCategory) {}

    void runTest() override
    {
        addingFilesKeepsOneEntryPerFile();
        assignmentsRejectInvalidInput();
        removingAnEntryDropsItsAssignments();
        clearingAnAssignmentDropsAnEmptyStyle();
        importingCopiesAndOverwrites();
        savingAndLoadingRoundTrips();
        generalMidiNamesCoverTheRange();
        loadingAMissingOrCorruptFileEmptiesTheLibrary();
        importingFromNowhereOrItselfChangesNothing();
    }

private:
    // An SFZ file that exists for the duration of one test.
    struct TemporarySfz
    {
        juce::TemporaryFile file { SfzLibraryTestValues::sfzExtension };
        TemporarySfz() { file.getFile().replaceWithText (SfzLibraryTestValues::sfzFileContents); }
        juce::File get() const { return file.getFile(); }
    };

    void addingFilesKeepsOneEntryPerFile()
    {
        using namespace SfzLibraryTestValues;
        using namespace SfzLibraryTestText;
        beginTest (addTitle);

        SFZLibraryManager library;
        const auto missing = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (missingSfzName);
        expect (library.addFile (missing).isEmpty(), missingFileAccepted);

        TemporarySfz piano;
        const auto firstId = library.addFile (piano.get());
        expect (firstId.isNotEmpty(), existingFileRejected);
        expect (library.addFile (piano.get()) == firstId, duplicateAddedTwice);
        expect (library.getEntries().size() == oneLibraryEntry, unexpectedEntryCount);
    }

    void assignmentsRejectInvalidInput()
    {
        using namespace SfzLibraryTestValues;
        using namespace SfzLibraryTestText;
        beginTest (assignTitle);

        SFZLibraryManager library;
        TemporarySfz piano;
        const auto pianoId = library.addFile (piano.get());

        library.assignToStyleInstrument (popStyle, pianoInstrument, pianoId);
        expect (library.getSfzForStyleInstrument (popStyle, pianoInstrument) == piano.get(), validAssignmentLost);

        library.assignToStyleInstrument (emptyStyle, pianoInstrument, pianoId);
        library.assignToStyleInstrument (popStyle, belowGmRange, pianoId);
        library.assignToStyleInstrument (popStyle, aboveGmRange, pianoId);
        library.assignToStyleInstrument (popStyle, bassInstrument, unknownEntryId);

        expect (library.getSfzForStyleInstrument (emptyStyle, pianoInstrument) == juce::File(), invalidAssignmentKept);
        expect (library.getSfzForStyleInstrument (popStyle, belowGmRange) == juce::File(), invalidAssignmentKept);
        expect (library.getSfzForStyleInstrument (popStyle, aboveGmRange) == juce::File(), invalidAssignmentKept);
        expect (library.getSfzForStyleInstrument (popStyle, bassInstrument) == juce::File(), invalidAssignmentKept);
    }

    void removingAnEntryDropsItsAssignments()
    {
        using namespace SfzLibraryTestValues;
        using namespace SfzLibraryTestText;
        beginTest (removeTitle);

        SFZLibraryManager library;
        TemporarySfz piano, bass;
        const auto pianoId = library.addFile (piano.get());
        const auto bassId  = library.addFile (bass.get());
        library.assignToStyleInstrument (popStyle, pianoInstrument, pianoId);
        library.assignToStyleInstrument (jazzStyle, pianoInstrument, pianoId);
        library.assignToStyleInstrument (jazzStyle, bassInstrument, bassId);

        library.removeEntry (pianoId);

        expect (library.getEntryById (pianoId) == nullptr, removedEntryStillUsed);
        expect (library.getSfzForStyleInstrument (jazzStyle, pianoInstrument) == juce::File(), removedEntryStillUsed);
        expect (library.getSfzForStyleInstrument (jazzStyle, bassInstrument) == bass.get(), otherAssignmentLost);
        expect (library.getData().styleMappings.find (popStyle) == library.getData().styleMappings.end(), emptyStyleKept);
    }

    void clearingAnAssignmentDropsAnEmptyStyle()
    {
        using namespace SfzLibraryTestValues;
        using namespace SfzLibraryTestText;
        beginTest (clearTitle);

        SFZLibraryManager library;
        TemporarySfz piano, bass;
        const auto pianoId = library.addFile (piano.get());
        const auto bassId  = library.addFile (bass.get());
        library.assignToStyleInstrument (popStyle, pianoInstrument, pianoId);
        library.assignToStyleInstrument (popStyle, bassInstrument, bassId);

        library.clearStyleInstrumentAssignment (popStyle, pianoInstrument);
        expect (library.getSfzForStyleInstrument (popStyle, pianoInstrument) == juce::File(), invalidAssignmentKept);
        expect (library.getSfzForStyleInstrument (popStyle, bassInstrument) == bass.get(), otherAssignmentLost);

        library.clearStyleInstrumentAssignment (popStyle, bassInstrument);
        expect (library.getData().styleMappings.find (popStyle) == library.getData().styleMappings.end(), emptyStyleKept);
    }

    void importingCopiesAndOverwrites()
    {
        using namespace SfzLibraryTestValues;
        using namespace SfzLibraryTestText;
        beginTest (importTitle);

        SFZLibraryManager library;
        TemporarySfz pianoA, pianoB, bass;
        const auto pianoAId = library.addFile (pianoA.get());
        const auto pianoBId = library.addFile (pianoB.get());
        const auto bassId   = library.addFile (bass.get());

        library.assignToStyleInstrument (popStyle, pianoInstrument, pianoAId);
        library.assignToStyleInstrument (jazzStyle, pianoInstrument, pianoBId);
        library.assignToStyleInstrument (jazzStyle, bassInstrument, bassId);

        library.importMappingsFromStyle (jazzStyle, popStyle);

        expect (library.getSfzForStyleInstrument (popStyle, bassInstrument) == bass.get(), importMissing);
        expect (library.getSfzForStyleInstrument (popStyle, pianoInstrument) == pianoB.get(), importDidNotOverwrite);
        expect (library.getSfzForStyleInstrument (jazzStyle, pianoInstrument) == pianoB.get(), targetOnlyLost);
    }

    void savingAndLoadingRoundTrips()
    {
        using namespace SfzLibraryTestValues;
        using namespace SfzLibraryTestText;
        beginTest (roundTripTitle);

        TemporarySfz piano, bass;
        juce::TemporaryFile libraryFile { jsonExtension };

        SFZLibraryManager saved;
        const auto pianoId = saved.addFile (piano.get());
        const auto bassId  = saved.addFile (bass.get());
        saved.assignToStyleInstrument (popStyle, pianoInstrument, pianoId);
        saved.assignToStyleInstrument (jazzStyle, lastGmInstrument, bassId);
        saved.save (libraryFile.getFile());

        SFZLibraryManager loaded;
        loaded.load (libraryFile.getFile());
        loaded.load (libraryFile.getFile());   // loading again must not duplicate anything

        expect (loaded.getEntries().size() == saved.getEntries().size(), unexpectedEntryCount);
        for (const auto& entry : saved.getEntries())
        {
            const auto* restored = loaded.getEntryById (entry.id);
            expect (restored != nullptr && restored->name == entry.name && restored->sfzPath == entry.sfzPath, roundTripDiffers);
        }
        expect (loaded.getSfzForStyleInstrument (popStyle, pianoInstrument) == piano.get(), roundTripDiffers);
        expect (loaded.getSfzForStyleInstrument (jazzStyle, lastGmInstrument) == bass.get(), roundTripDiffers);
        expect (loaded.getData().styleMappings.size() == saved.getData().styleMappings.size(), roundTripDiffers);
    }

    void generalMidiNamesCoverTheRange()
    {
        using namespace SfzLibraryTestValues;
        using namespace SfzLibraryTestText;
        beginTest (gmNamesTitle);

        expect (getGMInstrumentName (pianoInstrument) == firstGmName, gmNameWrong);
        expect (getGMInstrumentName (lastGmInstrument) == lastGmName, gmNameWrong);
        expect (getGMInstrumentName (belowGmRange).startsWith (outOfRangePrefix), gmNameWrong);
        expect (getGMInstrumentName (aboveGmRange).startsWith (outOfRangePrefix), gmNameWrong);
        expect (getGMInstrumentNames().size() == generalMidiInstrumentCount, gmNameWrong);
    }
    void loadingAMissingOrCorruptFileEmptiesTheLibrary()
    {
        using namespace SfzLibraryTestValues;
        using namespace SfzLibraryTestText;
        beginTest (badFileTitle);

        TemporarySfz piano;
        SFZLibraryManager library;
        library.assignToStyleInstrument (popStyle, pianoInstrument, library.addFile (piano.get()));

        library.load (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (missingJsonName));
        expect (library.getEntries().empty() && library.getData().styleMappings.empty(), staleDataAfterBadLoad);

        library.assignToStyleInstrument (popStyle, pianoInstrument, library.addFile (piano.get()));
        juce::TemporaryFile corruptFile { jsonExtension };
        corruptFile.getFile().replaceWithText (corruptJson);
        library.load (corruptFile.getFile());
        expect (library.getEntries().empty() && library.getData().styleMappings.empty(), staleDataAfterBadLoad);
    }

    void importingFromNowhereOrItselfChangesNothing()
    {
        using namespace SfzLibraryTestValues;
        using namespace SfzLibraryTestText;
        beginTest (importNoOpTitle);

        TemporarySfz piano;
        SFZLibraryManager library;
        library.assignToStyleInstrument (popStyle, pianoInstrument, library.addFile (piano.get()));
        const auto before = library.getData().styleMappings;

        library.importMappingsFromStyle (missingStyle, popStyle);
        library.importMappingsFromStyle (popStyle, popStyle);

        expect (library.getData().styleMappings == before, noOpImportChanged);
        expect (library.getData().styleMappings.find (missingStyle) == library.getData().styleMappings.end(), noOpImportChanged);
    }
};

static SfzLibraryTest sfzLibraryTest;
