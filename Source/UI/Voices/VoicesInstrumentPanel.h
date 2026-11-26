#pragma once
#include <JuceHeader.h>
#include <optional>
#include "InstrumentCatalog.h"
#include "VoicesPageLayout.h"
#include "UI/Stage/StagePageWidgets.h"

/*  The Voices page's instrument column (board StageRevised openPage=voices): the heading names the hand(s) being
    edited, the search covers every group, the group list picks a group and the instrument list shows its programs,
    each tagged R / L when that hand plays it. A click on an instrument reports its program; the owner applies it. */

/** Which hands are lit (the Part LEDs / the title row's Left hand / Right hand) and what each plays now. */
struct HandVoices
{
    bool leftLit {};
    bool rightLit {};
    int leftProgram {};
    int rightProgram {};
};

namespace VoicesModel
{
    /** "Right hand", "Left hand", "Both hands" or "No hand": the heading's second half. */
    juce::String editedHands (const HandVoices& hands);

    /** The tag at the right of an instrument row: "R", "L", "L R" or nothing. */
    juce::String handTags (int program, const HandVoices& hands);

    /** The program shown selected: the right hand's when it is lit, else the left hand's, else none. */
    std::optional<int> editedProgram (const HandVoices& hands);

    /** The group the page opens on: the edited program's, else the right hand's, else the first. */
    size_t openingGroup (const HandVoices& hands);
}

class VoicesInstrumentPanel : public juce::Component
{
public:
    VoicesInstrumentPanel();
    ~VoicesInstrumentPanel() override;

    std::function<void (int program)> onInstrumentChosen;

    void setScale (float newScale);
    void setHands (const HandVoices& newHands);

    /** The page opens: search cleared, the opening group shown. */
    void showOpeningGroup();

    juce::String getHeading() const;
    std::optional<size_t> getSelectedGroup() const;
    const std::vector<CatalogInstrument>& getShownInstruments() const;
    void setSearchText (const juce::String& text);
    void chooseGroup (size_t groupIndex);
    void chooseShownInstrument (size_t shownIndex);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    struct GroupRows;
    struct InstrumentRows;

    void refreshShown();
    InstrumentColumnRects layout() const;

    HandVoices hands;
    std::optional<size_t> selectedGroup { size_t {} };
    std::vector<CatalogInstrument> shown;
    float scale { StageMetrics::referenceScale };

    StageLcdInput search;
    std::unique_ptr<GroupRows> groupRows;
    std::unique_ptr<InstrumentRows> instrumentRows;
    juce::ListBox groupList, instrumentList;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoicesInstrumentPanel)
};
