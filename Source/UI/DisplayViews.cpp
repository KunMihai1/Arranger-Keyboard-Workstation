/*
  ==============================================================================

    DisplayViews.cpp

    The Style page's views (board StageRevised openPage=styles, PageTracks,
    PageNotes): the style itself (styles list + track strips), and the pages
    opened from it - the track picker, the notes page and the style settings -
    shown one at a time in the same page (StyleViewRouter). Split from
    Display.cpp to keep both under the file cap (ADR-003).

  ==============================================================================
*/

#include "Display.h"
#include "UI/Stage/StageMetrics.h"

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
    playBackSettings.reset();
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
    if (playBackSettings)
        playBackSettings->setBounds (area);
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
        case StyleView::tracks:   return StylePageText::tracksTitle;
        case StyleView::settings: return StylePageText::settingsTitle;
        case StyleView::notes:
        case StyleView::style:    break;
    }
    return StylePageText::title;
}

juce::String Display::getViewSubtitle() const
{
    return {};
}

std::vector<juce::Component*> Display::getTitleActions()
{
    if (views.current() == StyleView::style)
        return { &playLabel, &allTracksButton, &soloButton, &playDivider, &sectionsButton, &styleSettingsButton };
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

void Display::openStyleSettings()
{
    if (currentStyleComponent == nullptr)
        return;

    destroySubView();
    playBackSettings = std::make_unique<PlayBackSettingsComponent>(minNote, maxNote, this->settings);
    playBackSettings->getStyleID = [this]() { return this->getStyleID(); };

    playBackSettings->onChangingSettings = [this](PlayBackSettings)
    {
        // Save settings using PropertiesFile
        PlaybackSettingsIOHelper::savePlaybackSettings(propertiesFile, this->settings, minNote, maxNote, this->getStyleID());
        displayListeners.call(&DisplayListener::playBackSettingsChanged, this->settings);
    };

    playBackSettings->onChangingTranspose = [this](int transposeValue)
    {
        displayListeners.call(&DisplayListener::playBackSettingsTransposeChanged, transposeValue);
    };

    addAndMakeVisible (playBackSettings.get());
    showView (StyleView::settings);
}
