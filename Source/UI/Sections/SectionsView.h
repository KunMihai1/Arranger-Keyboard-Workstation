#pragma once
#include <JuceHeader.h>
#include "Arranger/ArrangerStyleListComponent.h"
#include "Arranger/ArrangerStyleEditor.h"

/*  The Sections view of the Style page (board PageSections; spec 2026-11-27 sections v2): the configurations
    list in the left column and, while one is being edited, the editor over the rest. It only hosts them: the
    list's and editor's owner (CurrentStyleComponent) keeps all their logic, as when they were overlays. */

class SectionsView : public juce::Component
{
public:
    explicit SectionsView (ArrangerStyleListComponent& listToShow);

    void setScale (float newScale);
    /** The editor to show (nullptr: none; the centre then says how to start). */
    void showEditor (ArrangerStyleEditor* editorToShow);
    ArrangerStyleEditor* getEditor() const { return editor; }
    /** The title row's controls: the editor's while one is open. */
    std::vector<juce::Component*> getTitleActions();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    ArrangerStyleListComponent& list;
    ArrangerStyleEditor* editor = nullptr;
    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SectionsView)
};
