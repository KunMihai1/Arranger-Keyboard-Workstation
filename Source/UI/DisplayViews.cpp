/*
  ==============================================================================

    DisplayViews.cpp

    The Style page's views (board StageRevised openPage=styles, PageTracks,
    PageNotes): the style itself (styles list + track strips), and the pages
    opened from it - the track picker and the notes page - shown one at a time
    in the same page (StyleViewRouter). "Style settings..." opens the Voices page. Split from
    Display.cpp to keep both under the file cap (ADR-003).

  ==============================================================================
*/

#include "Display.h"
#include "UI/Stage/StageMetrics.h"
#include "CustomTableContainer.h"

StylePageRects stylePageLayout (juce::Rectangle<int> content, float scale)
{
    using namespace StageMetrics;
    const auto column = juce::roundToInt (static_cast<float> (stylesColumnWidth) * scale);
    const auto gap    = juce::roundToInt (static_cast<float> (stylesColumnGap) * scale);
    StylePageRects rects;
    rects.stylesColumn = content.withWidth (column);
    rects.tracksArea   = content.withTrimmedLeft (column + gap);
    return rects;
}

void Display::setScale (float newScale)
{
    scale = newScale;
    stylesListComponent->setScale (scale);
    if (currentStyleComponent)
        currentStyleComponent->setScale (scale);
    if (trackListComp)
        trackListComp->setScale (scale);
    if (notesContainer)
        notesContainer->setScale (scale);
    if (instrumentPicker)
        instrumentPicker->setScale (scale);
    layoutViews();
    repaint();
}

StyleView Display::getView() const
{
    return views.current();
}

void Display::showView (StyleView view)
{
    views.show (view);
    layoutViews();
    viewChanged();
}

void Display::closeSubView()
{
    destroySubView();
    views.back();
    layoutViews();
    refreshTitleActions();
    viewChanged();
}

void Display::resetView()
{
    if (views.current() == StyleView::style)
        return;
    closeSubView();
}

void Display::destroySubView()
{
    if (trackListComp)
    {
        trackListComp->removeListener (this);
        trackListComp.reset();
    }
    notesContainer.reset();
    instrumentPicker.reset();
}

void Display::layoutViews()
{
    const auto area  = getLocalBounds();
    const bool style = views.current() == StyleView::style;
    const auto rects = stylePageLayout (area, scale);

    stylesListComponent->setVisible (style);
    stylesListComponent->setBounds (rects.stylesColumn);
    if (currentStyleComponent)
    {
        currentStyleComponent->setVisible (style);
        currentStyleComponent->setBounds (rects.tracksArea);
    }

    if (trackListComp)
        trackListComp->setBounds (area);
    if (notesContainer)
        notesContainer->setBounds (area);
    if (instrumentPicker)
        instrumentPicker->setBounds (area);
    repaint();
}

void Display::paint (juce::Graphics& g)
{
    // No style open yet: the tracks area shows its heading and where to start.
    if (views.current() != StyleView::style || currentStyleComponent != nullptr)
        return;

    const auto tracks = stylePageLayout (getLocalBounds(), scale).tracksArea.toFloat();
    StageLcd::paintHeading (g, tracks.withHeight (static_cast<float> (StageMetrics::headingHeight) * scale), scale,
                            StylePageText::tracks, StylePageText::openAStyle);
}

juce::String Display::getViewTitle() const
{
    switch (views.current())
    {
        case StyleView::tracks:
            return StylePageText::tracksTitle + juce::String::fromUTF8 (StylePageText::slotSeparatorUtf8)
                 + juce::String (currentStyleComponent ? currentStyleComponent->getRequestingSlot() : int {});
        case StyleView::instruments:
            return StylePageText::instrumentsTitle + juce::String::fromUTF8 (StylePageText::slotSeparatorUtf8) + juce::String (instrumentSlot);
        case StyleView::notes:    return notesTitle;
        case StyleView::style:    break;
    }
    return StylePageText::title;
}

juce::String Display::getViewSubtitle() const
{
    return views.current() == StyleView::notes ? notesSubtitle : juce::String();
}

void Display::showNotes (std::unique_ptr<TableContainer> notes, const juce::String& title, const juce::String& subtitle)
{
    destroySubView();   // one sub-page at a time
    notesContainer = std::move (notes);
    notesTitle     = title;
    notesSubtitle  = subtitle;
    notesContainer->setScale (scale);
    addAndMakeVisible (notesContainer.get());
    showView (StyleView::notes);
}

void Display::showInstrumentPicker (int slot, int currentProgram, std::function<void (int)> onChosen)
{
    destroySubView();   // one sub-page at a time
    instrumentSlot = slot;
    instrumentPicker = std::make_unique<InstrumentPicker> (currentProgram);
    instrumentPicker->setScale (scale);
    instrumentPicker->onChosen = [this, onChosen] (int program)
    {
        if (onChosen)
            onChosen (program);
        // Back to the style. Not now: this runs inside the picker's own click, which must not delete the
        // picker under itself.
        const juce::Component::SafePointer<Display> safeThis (this);
        juce::MessageManager::callAsync ([safeThis]
        {
            if (safeThis != nullptr && safeThis->getView() == StyleView::instruments)
                safeThis->closeSubView();
        });
    };
    addAndMakeVisible (instrumentPicker.get());
    showView (StyleView::instruments);
}

std::vector<juce::Component*> Display::getTitleActions()
{
    if (views.current() == StyleView::style)
        return { &playLabel, &allTracksButton, &soloButton, &playDivider, &sectionsButton, &styleSettingsButton };
    if (views.current() == StyleView::tracks && trackListComp)
        return trackListComp->getTitleActions();
    return {};
}

void Display::refreshTitleActions()
{
    const bool open = currentStyleComponent != nullptr;
    const bool solo = open && currentStyleComponent->isPlayModeSolo();
    for (auto* button : { &allTracksButton, &soloButton, &sectionsButton, &styleSettingsButton })
        button->setEnabled (open);
    allTracksButton.setLedOn (open && ! solo);
    soloButton.setLedOn (solo);
}

void Display::viewChanged()
{
    if (onViewChanged)
        onViewChanged();
}

PlayBackSettingsComponent& Display::getPlaybackSettings()
{
    return *playBackSettings;
}
