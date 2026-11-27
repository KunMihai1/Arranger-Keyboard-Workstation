#include "SectionsView.h"
#include "SectionsPageLayout.h"
#include "Common/AppColours.h"
#include "UI/Stage/StageFonts.h"

namespace
{
    const juce::String emptyHint = "Pick a configuration and press Edit, or New to cut one from your recording.";
    constexpr float hintSize    = 11.0f;   // .silk-dim at the board's hint size
    constexpr float hintSpacing = 0.06f;
    constexpr bool  mediumFace  = false;
    constexpr int   noLanes     = 0;
}

SectionsView::SectionsView (ArrangerStyleListComponent& listToShow) : list (listToShow)
{
    addAndMakeVisible (list);
}

void SectionsView::setScale (float newScale)
{
    scale = newScale;
    list.setScale (scale);
    if (editor != nullptr)
        editor->setScale (scale);
    resized();
    repaint();
}

void SectionsView::showEditor (ArrangerStyleEditor* editorToShow)
{
    if (editor != nullptr)
        removeChildComponent (editor);
    editor = editorToShow;
    list.setEnabled (true);
    if (editor != nullptr)
    {
        addAndMakeVisible (editor);
        editor->setScale (scale);
        editor->onBusyChanged = [this] (bool busy) { list.setEnabled (! busy); };   // a save in progress blocks the list too
        list.toFront (false);   // the editor spans the page; the list keeps its corner of the left column
    }
    resized();
    repaint();
}

std::vector<juce::Component*> SectionsView::getTitleActions()
{
    return editor != nullptr ? editor->getTitleActions() : std::vector<juce::Component*> {};
}

void SectionsView::resized()
{
    const auto rects = sectionsPageLayout (getLocalBounds(), scale, noLanes);
    list.setBounds (rects.configHeading.getUnion (rects.configButtons));
    if (editor != nullptr)
        editor->setBounds (getLocalBounds());
}

void SectionsView::paint (juce::Graphics& g)
{
    if (editor != nullptr)
        return;
    const auto rects = sectionsPageLayout (getLocalBounds(), scale, noLanes);
    g.setColour (StagePalette::silkscreenDimText);
    g.setFont (StageFonts::silkscreen (hintSize * scale, mediumFace, hintSpacing));
    g.drawText (emptyHint, rects.centre.withBottom (getHeight()).getUnion (rects.rightColumn.withTop (rects.centre.getY())),
                juce::Justification::centred, true);
}
