#include <JuceHeader.h>
#include "UI/Stage/StagePanel.h"
#include "UI/Stage/StageMetrics.h"
#include "Styles/SectionGroupComponent.h"

namespace StagePanelTestValues
{
    const juce::String testName     = "StagePanel";
    const juce::String testCategory = "Unit";

    const juce::String sectionNames   = "section buttons report the arranger's own names";
    const juce::String oneSectionLit  = "only the playing section's LED is lit";
    const juce::String transport      = "transport buttons report presses and show state";
    const juce::String pages          = "page buttons report their page; the open page's LED is lit";
    const juce::String tempoHold      = "holding Tempo + steps at once and repeats until release; the style hears once, on release (review: every step re-scaled all tracks)";
    const juce::String quickFx        = "quick knobs and part toggles report changes";
    const juce::String roomyLayout    = "buttons sit >= 20 px apart and everything is inside the panel";
    const juce::String centredContent = "on a wide window the controls stay the board's width, centred";
    const juce::String opaquePanel    = "the panel is opaque, so a knob repaint never redraws the window behind it";
    const juce::String knobRelease    = "a quick knob reports each value while turning and once more on release";
    const juce::String knobsRevert    = "a quick knob turned with no part chosen goes back to what it showed (review: it stayed moved, sound unchanged)";
    constexpr int    shownVolume  = 110;
    constexpr int    shownReverb  = 72;
    constexpr int    turnedValue  = 20;
    constexpr size_t oneReport    = 1;

    const juce::String varTwo = "Var 2";
    const juce::String listSeparator = ", ";

    constexpr double styleTempo   = 120.0;
    constexpr double fasterTempo  = 121.0;
    constexpr float  ownerScale   = 1.6f;
    constexpr int    ownerWidth   = 1920;
    constexpr int    ownerPanel   = 198;
    constexpr int    wideWidth    = 2560;
    constexpr float  wideScale    = 1.6744186f;   // 1080 / 645
    constexpr int    widePanel    = 208;
    constexpr int    knobValue    = 100;
    constexpr float  minimumCapGap = 20.0f;       // owner: buttons were too tight
    constexpr float  layoutSlack  = 0.5f;
    constexpr double tolerance    = 0.0001;
    constexpr int    firstSection = 1;
    constexpr int    threeSections = 3;
    constexpr int    fourSections  = 4;
    constexpr int    nextIndex     = 1;
    constexpr int    centreSlack   = 1;   // px: centring an odd width
}

class StagePanelTest : public juce::UnitTest
{
    static RubberButton* buttonNamed (StagePanel& panel, const juce::String& buttonName)
    {
        for (auto* child : panel.getChildren())
            if (auto* button = dynamic_cast<RubberButton*> (child); button != nullptr && button->getName() == buttonName)
                return button;
        return nullptr;
    }

    static StageKnob* knobNamed (StagePanel& panel, const juce::String& label)
    {
        for (auto* child : panel.getChildren())
            if (auto* knob = dynamic_cast<StageKnob*> (child); knob != nullptr && knob->getLabel() == label)
                return knob;
        return nullptr;
    }

    static juce::StringArray allSectionNames()
    {
        using namespace StagePanelTestValues;
        juce::StringArray names;
        for (int number = firstSection; number <= threeSections; ++number) names.add (SectionGroupNames::introPrefix + juce::String (number));
        for (int number = firstSection; number <= threeSections; ++number) names.add (SectionGroupNames::endingPrefix + juce::String (number));
        for (int number = firstSection; number <= fourSections; ++number)  names.add (SectionGroupNames::variationPrefix + juce::String (number));
        names.add (SectionGroupNames::breakButton);
        for (int number = firstSection; number <= fourSections; ++number)  names.add (SectionGroupNames::fillPrefix + juce::String (number));
        return names;
    }

    void click (StagePanel& panel, const juce::String& buttonName)
    {
        auto* button = buttonNamed (panel, buttonName);
        expect (button != nullptr, buttonName);
        if (button != nullptr && button->onClick != nullptr)
            button->onClick();
    }

    void expectOnlyLit (StagePanel& panel, const juce::StringArray& group, const juce::String& litName)
    {
        for (const auto& buttonName : group)
            if (auto* button = buttonNamed (panel, buttonName))
                expect (button->isLedOn() == (buttonName == litName), buttonName);
    }

public:
    StagePanelTest() : juce::UnitTest (StagePanelTestValues::testName, StagePanelTestValues::testCategory) {}

    void runTest() override
    {
        using namespace StagePanelTestValues;

        beginTest (sectionNames);
        {
            StagePanel panel { styleTempo };
            juce::StringArray reported;
            panel.onSection = [&reported] (const juce::String& buttonName) { reported.add (buttonName); };
            for (const auto& buttonName : allSectionNames())
                click (panel, buttonName);
            expect (reported == allSectionNames(), reported.joinIntoString (listSeparator));
        }

        beginTest (oneSectionLit);
        {
            StagePanel panel { styleTempo };
            expect (buttonNamed (panel, varTwo) != nullptr);
            panel.setSectionLit (varTwo);
            expectOnlyLit (panel, allSectionNames(), varTwo);
            panel.setSectionLit ({});
            expectOnlyLit (panel, allSectionNames(), {});
        }

        beginTest (transport);
        {
            StagePanel panel { styleTempo };
            juce::StringArray pressed;
            panel.onAutoFill  = [&pressed] { pressed.add (StagePanelNames::autoFill); };
            panel.onRec       = [&pressed] { pressed.add (StagePanelNames::rec); };
            panel.onFile      = [&pressed] { pressed.add (StagePanelNames::file); };
            panel.onSyncStart = [&pressed] { pressed.add (StagePanelNames::syncStart); };
            panel.onStop      = [&pressed] { pressed.add (StagePanelNames::stop); };
            panel.onStart     = [&pressed] { pressed.add (StagePanelNames::start); };
            const juce::StringArray order { StagePanelNames::autoFill, StagePanelNames::rec, StagePanelNames::file,
                                            StagePanelNames::syncStart, StagePanelNames::stop, StagePanelNames::start };
            for (const auto& buttonName : order)
                click (panel, buttonName);
            expect (pressed == order);

            panel.setRecording (true);
            panel.setPlaying (true);
            panel.setAutoFill (true);
            panel.setSyncStart (true);
            for (const auto& buttonName : { StagePanelNames::rec, StagePanelNames::start, StagePanelNames::autoFill, StagePanelNames::syncStart })
                if (auto* button = buttonNamed (panel, buttonName))
                    expect (button->isLedOn(), buttonName);
            panel.setRecording (false);
            if (auto* recButton = buttonNamed (panel, StagePanelNames::rec))
                expect (! recButton->isLedOn());
        }

        beginTest (pages);
        {
            StagePanel panel { styleTempo };
            std::vector<StagePage> reported;
            panel.onPage = [&reported] (StagePage page) { reported.push_back (page); };
            const std::vector<std::pair<juce::String, StagePage>> pageOf {
                { StagePanelNames::home, StagePage::home },         { StagePanelNames::styles, StagePage::styles },
                { StagePanelNames::voices, StagePage::voices },     { StagePanelNames::effects, StagePage::effects },
                { StagePanelNames::settings, StagePage::settings }, { StagePanelNames::chords, StagePage::chords } };

            for (const auto& [buttonName, page] : pageOf)
                click (panel, buttonName);
            expectEquals (static_cast<int> (reported.size()), static_cast<int> (pageOf.size()));
            for (size_t pageIndex {}; pageIndex < juce::jmin (reported.size(), pageOf.size()); ++pageIndex)
                expect (reported[pageIndex] == pageOf[pageIndex].second, pageOf[pageIndex].first);

            juce::StringArray pageButtons { StagePanelNames::file };
            for (const auto& entry : pageOf)
                pageButtons.add (entry.first);

            panel.setPageLed (StagePage::effects);
            expectOnlyLit (panel, pageButtons, StagePanelNames::effects);
            panel.setPageLed (StagePage::file);
            expectOnlyLit (panel, pageButtons, StagePanelNames::file);
            panel.setPageLed (StagePage::none);
            expectOnlyLit (panel, pageButtons, {});
        }

        beginTest (tempoHold);
        {
            StagePanel panel { styleTempo };
            std::vector<double> tempos;
            panel.screen().onTempoChanged = [&tempos] (double tempo) { tempos.push_back (tempo); };
            auto* tempoUp = buttonNamed (panel, StagePanelNames::tempoUp);
            expect (tempoUp != nullptr);
            if (tempoUp != nullptr)
            {
                tempoUp->setState (juce::Button::buttonDown);
                expect (panel.isHoldingTempo());
                expect (std::abs (panel.screen().tempo().value() - fasterTempo) < tolerance);
                expect (tempos.empty());
                tempoUp->setState (juce::Button::buttonNormal);
                expect (! panel.isHoldingTempo());
                expect (tempos.size() == oneReport && std::abs (tempos.back() - fasterTempo) < tolerance);
            }
        }

        beginTest (knobsRevert);
        {
            StagePanel panel { styleTempo };
            panel.setQuickKnobValues (shownVolume, shownReverb);
            auto* volume = knobNamed (panel, StagePanelNames::volume);
            auto* reverb = knobNamed (panel, StagePanelNames::reverb);
            expect (volume != nullptr && reverb != nullptr);
            if (volume != nullptr && reverb != nullptr)
            {
                volume->setValue (turnedValue, juce::sendNotificationSync);
                reverb->setValue (turnedValue, juce::sendNotificationSync);
                panel.revertQuickKnobs();
                expectEquals (juce::roundToInt (volume->getValue()), shownVolume);
                expectEquals (juce::roundToInt (reverb->getValue()), shownReverb);
            }
        }

        beginTest (quickFx);
        {
            StagePanel panel { styleTempo };
            std::vector<std::pair<StagePanel::QuickKnob, int>> knobs;
            std::vector<bool> parts;
            panel.onQuickKnob = [&knobs] (StagePanel::QuickKnob knob, int value) { knobs.emplace_back (knob, value); };
            panel.onPart = [&parts] (bool left) { parts.push_back (left); };

            if (auto* volume = knobNamed (panel, StagePanelNames::volume))
                volume->setValue (knobValue, juce::sendNotificationSync);
            expect (! knobs.empty() && knobs.back().first == StagePanel::QuickKnob::volume && knobs.back().second == knobValue);
            if (auto* reverb = knobNamed (panel, StagePanelNames::reverb))
                reverb->setValue (knobValue, juce::sendNotificationSync);
            expect (! knobs.empty() && knobs.back().first == StagePanel::QuickKnob::reverb);

            click (panel, StagePanelNames::partLeft);
            click (panel, StagePanelNames::partRight);
            expect (parts == std::vector<bool> { true, false });

            panel.setPartLeds (true, false);
            if (auto* left = buttonNamed (panel, StagePanelNames::partLeft))   expect (left->isLedOn());
            if (auto* right = buttonNamed (panel, StagePanelNames::partRight)) expect (! right->isLedOn());

            const auto valueBefore = knobs.size();
            panel.setQuickKnobValues (knobValue, knobValue);   // showing state must not report a change
            expectEquals (static_cast<int> (knobs.size()), static_cast<int> (valueBefore));
        }

        beginTest (opaquePanel);
        {
            StagePanel panel { styleTempo };
            expect (panel.isOpaque());
        }

        beginTest (knobRelease);
        {
            StagePanel panel { styleTempo };
            std::vector<int> live, released;
            panel.onQuickKnob = [&live] (StagePanel::QuickKnob, int value) { live.push_back (value); };
            panel.onQuickKnobReleased = [&released] (StagePanel::QuickKnob, int value) { released.push_back (value); };
            auto* volume = knobNamed (panel, StagePanelNames::volume);
            expect (volume != nullptr);
            if (volume != nullptr)
            {
                volume->setValue (knobValue, juce::sendNotificationSync);
                expect (released.empty());
                if (volume->onDragEnd != nullptr)
                    volume->onDragEnd();
                expect (released == std::vector<int> { knobValue });
                expect (live == std::vector<int> { knobValue });
            }
        }

        beginTest (roomyLayout);
        {
            StagePanel panel { styleTempo };
            panel.setScale (ownerScale);
            panel.setSize (ownerWidth, ownerPanel);
            expect (buttonNamed (panel, varTwo) != nullptr);

            for (auto* child : panel.getChildren())
                if (child->isVisible())
                    expect (panel.getLocalBounds().contains (child->getBounds()), child->getName());

            // Neighbouring section caps on the same row: at least 20 board px of clear space.
            const auto names = allSectionNames();
            for (int nameIndex {}; nameIndex + nextIndex < names.size(); ++nameIndex)
            {
                auto* leftButton  = buttonNamed (panel, names[nameIndex]);
                auto* rightButton = buttonNamed (panel, names[nameIndex + nextIndex]);
                if (leftButton == nullptr || rightButton == nullptr)
                    continue;
                const auto leftCap  = leftButton->getCapBounds() + leftButton->getPosition().toFloat();
                const auto rightCap = rightButton->getCapBounds() + rightButton->getPosition().toFloat();
                if (std::abs (leftCap.getY() - rightCap.getY()) > layoutSlack)
                    continue;   // next row
                expectGreaterOrEqual (rightCap.getX() - leftCap.getRight(), minimumCapGap * ownerScale - layoutSlack);
            }
        }

        beginTest (centredContent);
        {
            StagePanel panel { styleTempo };
            panel.setScale (wideScale);
            panel.setSize (wideWidth, widePanel);
            const auto content = panel.getContentBounds();
            expectEquals (content.getWidth(), juce::roundToInt (StageMetrics::referenceWidth * wideScale));
            expect (std::abs (content.getCentreX() - panel.getLocalBounds().getCentreX()) <= centreSlack);
            for (auto* child : panel.getChildren())
                if (child->isVisible())
                    expect (content.contains (child->getBounds()), child->getName());
        }
    }
};
static StagePanelTest stagePanelTest;
