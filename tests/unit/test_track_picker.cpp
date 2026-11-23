#include <JuceHeader.h>
#include "TrackListComponent.h"
#include "TrackListComponentNames.h"
#include "TrackEntry.h"

/*  The track picker page (board PageTracks): folder details on the right, the sort button cycling
    Name / Newest / Oldest, and the track rename dialog focusing its own box (backlog section 2: it looked
    up the folder dialog's box, found none and focused nothing). In-memory library only. */

namespace TrackPickerTestValues
{
    const juce::String testName     = "TrackPicker";
    const juce::String testCategory = "Unit";

    const juce::String folderDetails  = "a selected folder shows its name and its track counts by type";
    const juce::String sortCycles     = "the sort button steps Name -> Newest -> Oldest -> Name and the open folder follows";
    const juce::String renameFocus    = "the track rename dialog's box is found by the name the focus call uses";
    const juce::String useInSlot      = "the use button names the slot being filled";

    const juce::String ballads   = "Ballads";
    const juce::String folderKey = "Folder";
    const juce::String tracksKey = "Tracks";
    const juce::String melodicKey = "Melodic";
    const juce::String percussionKey = "Percussion";
    const juce::String nameSort   = "NAME";
    const juce::String newestSort = "NEWEST";
    const juce::String oldestSort = "OLDEST";
    const juce::String alpha = "Alpha";
    const juce::String beta  = "Beta";
    const juce::String gamma = "Gamma";
    const juce::String drums = "Drums";
    const juce::String slotWord = "slot 3";
    const juce::String noTitle;
    const juce::String noMessage;

    constexpr int melodicCount = 4;
    constexpr int percussionCount = 1;
    constexpr int totalCount = 5;
    constexpr int firstRow = 0;
    constexpr int slotThree = 3;
    constexpr int detailRows = 4;
    constexpr int folderRow = 0;
    constexpr int nameRow = 0;
    constexpr int tracksRow = 1;
    constexpr int melodicRow = 2;
    constexpr int percussionRow = 3;
    constexpr int oldestAgeDays = 3;
    constexpr int middleAgeDays = 2;
    constexpr int newestAgeDays = 1;
    constexpr int doubleClickCount = 2;
    constexpr float noOrientation = 0.0f;
    constexpr float noRotation = 0.0f;
    constexpr float noTilt = 0.0f;
}

class TrackPickerTest : public juce::UnitTest
{
public:
    TrackPickerTest() : juce::UnitTest (TrackPickerTestValues::testName, TrackPickerTestValues::testCategory) {}

    struct Library
    {
        std::shared_ptr<std::deque<TrackEntry>> openFolder = std::make_shared<std::deque<TrackEntry>>();
        std::shared_ptr<std::unordered_map<juce::String, std::deque<TrackEntry>>> folders = std::make_shared<std::unordered_map<juce::String, std::deque<TrackEntry>>>();
        std::shared_ptr<std::vector<juce::String>> keys = std::make_shared<std::vector<juce::String>>();
    };

    static TrackEntry entry (const juce::String& name, TrackType type, const juce::File& file)
    {
        TrackEntry track;
        track.displayName = name;
        track.type = type;
        track.uuid = juce::Uuid();
        track.file = file;
        return track;
    }

    static void openFolder (TrackListComponent& list, int row)
    {
        using namespace TrackPickerTestValues;
        const auto now = juce::Time::getCurrentTime();
        const juce::MouseEvent doubleClick (juce::Desktop::getInstance().getMainMouseSource(), {}, juce::ModifierKeys::leftButtonModifier,
                                            juce::MouseInputSource::defaultPressure, noOrientation, noRotation, noTilt, noTilt,
                                            &list, &list, now, {}, now, doubleClickCount, false);
        list.listBoxItemClicked (row, doubleClick);
    }

    void runTest() override
    {
        using namespace TrackPickerTestValues;

        beginTest (folderDetails);
        {
            Library lib;
            std::deque<TrackEntry> tracks;
            for (int melodic {}; melodic < melodicCount; ++melodic)
                tracks.push_back (entry (alpha + juce::String (melodic), TrackType::Melodic, {}));
            tracks.push_back (entry (drums, TrackType::Percussion, {}));
            (*lib.folders)[ballads] = tracks;
            lib.keys->push_back (ballads);

            TrackListComponent list (lib.openFolder, lib.folders, lib.keys, [] (int) {});
            list.selectRow (folderRow);
            const auto rows = list.details();
            expectEquals (static_cast<int> (rows.size()), detailRows);
            if (static_cast<int> (rows.size()) == detailRows)
            {
                expectEquals (rows[nameRow].label, folderKey);       expectEquals (rows[nameRow].value, ballads);
                expectEquals (rows[tracksRow].label, tracksKey);     expectEquals (rows[tracksRow].value, juce::String (totalCount));
                expectEquals (rows[melodicRow].label, melodicKey);   expectEquals (rows[melodicRow].value, juce::String (melodicCount));
                expectEquals (rows[percussionRow].label, percussionKey); expectEquals (rows[percussionRow].value, juce::String (percussionCount));
            }
        }

        beginTest (sortCycles);
        {
            // Three files with different ages: Alpha is the oldest, Gamma the newest.
            juce::TemporaryFile oldest, middle, newest;
            const auto now = juce::Time::getCurrentTime();
            for (auto [file, days] : { std::pair<const juce::TemporaryFile*, int> { &oldest, oldestAgeDays }, { &middle, middleAgeDays }, { &newest, newestAgeDays } })
            {
                file->getFile().create();
                file->getFile().setLastModificationTime (now - juce::RelativeTime::days (days));
            }

            Library lib;
            (*lib.folders)[ballads] = { entry (beta, TrackType::Melodic, middle.getFile()),
                                         entry (gamma, TrackType::Melodic, newest.getFile()),
                                         entry (alpha, TrackType::Melodic, oldest.getFile()) };
            lib.keys->push_back (ballads);
            TrackListComponent list (lib.openFolder, lib.folders, lib.keys, [] (int) {});
            openFolder (list, folderRow);

            expectEquals (list.sortCaption(), nameSort);
            expectEquals (list.getAllAvailableTracks()[firstRow].displayName, alpha);
            list.cycleSort();
            expectEquals (list.sortCaption(), newestSort);
            expectEquals (list.getAllAvailableTracks()[firstRow].displayName, gamma);
            list.cycleSort();
            expectEquals (list.sortCaption(), oldestSort);
            expectEquals (list.getAllAvailableTracks()[firstRow].displayName, alpha);
            list.cycleSort();
            expectEquals (list.sortCaption(), nameSort);
        }

        beginTest (renameFocus);
        {
            juce::AlertWindow window (noTitle, noMessage, juce::AlertWindow::NoIcon);
            TrackListComponent::addTrackRenameEditor (window, alpha);
            expect (TrackListComponent::trackRenameEditor (window) != nullptr);
        }

        beginTest (useInSlot);
        {
            Library lib;
            TrackListComponent list (lib.openFolder, lib.folders, lib.keys, [] (int) {});
            list.setSlotNumber (slotThree);
            expect (list.getUseButtonText().containsIgnoreCase (slotWord));
        }
    }
};
static TrackPickerTest trackPickerTest;
