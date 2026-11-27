#pragma once
#include <JuceHeader.h>
#include "Arranger/ArrangerStyleFile.h"
#include "UI/Stage/StagePageWidgets.h"
#include <optional>

/*  "Selected section" (board PageSections, right column): an LCD with the section's name, type, start bar and
    length (◂ ▸ step them), "When it ends" Loop / Go on / Stop, then Play this and Remove. It only reports what
    was pressed; the editor applies it (SectionEditModel) like a drag. Nothing selected: dashes, buttons off. */

enum class InspectorArrow { typeEarlier, typeLater, startEarlier, startLater, lengthShorter, lengthLonger };

class SectionInspector : public juce::Component
{
public:
    SectionInspector();

    void setSection (std::optional<SectionWindow> window);
    void setScale (float newScale);

    /** Where an arrow is drawn and clicked; empty while nothing is selected. */
    juce::Rectangle<int> arrowBounds (InspectorArrow arrow) const;
    void clickAt (juce::Point<int> position);
    SegButton& getAfterButton (ArrangerAfterComplete afterComplete);
    SegButton& getPlayThisButton() { return playThis; }
    SegButton& getRemoveButton() { return remove; }

    std::function<void (InspectorArrow)> onArrow;
    std::function<void (ArrangerAfterComplete)> onAfterComplete;
    std::function<void()> onPlayThis, onRemove;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;

private:
    juce::Rectangle<int> lcdBounds() const;
    juce::Rectangle<int> lcdRow (int rowIndex) const;
    juce::Font lcdFont() const;

    std::optional<SectionWindow> section;
    float scale { StageMetrics::referenceScale };
    SilkLabel heading, whenItEnds;
    SegButton loop, goOn, stop, playThis, remove;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SectionInspector)
};
