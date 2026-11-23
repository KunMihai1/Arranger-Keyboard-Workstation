/*
  ==============================================================================

    DisplayStage.cpp

    Display's small interface for the Stage panel: the open style's transport,
    tempo and state. Split from Display.cpp to keep both under the file cap
    (ADR-003). Without an open style every call does nothing / reports the
    defaults, which is today's behaviour: Start / Stop live on the style tab.

  ==============================================================================
*/

#include "Display.h"

bool Display::hasOpenStyle() const
{
    return currentStyleComponent != nullptr;
}

void Display::startOpenStyle()
{
    if (currentStyleComponent)
        currentStyleComponent->triggerStartClick();
}

void Display::stopOpenStyle()
{
    if (currentStyleComponent)
        currentStyleComponent->triggerStopClick();
}

bool Display::isOpenStylePlaying()
{
    return currentStyleComponent && currentStyleComponent->getIsPlaying();
}

bool Display::isOpenStyleInArrangerMode() const
{
    return currentStyleComponent && currentStyleComponent->isArrangerModeEnabled();
}

double Display::getOpenStyleTempo()
{
    return currentStyleComponent ? currentStyleComponent->getTempo() : ArrangerStyleDefaults::tempoBpm;
}

double Display::getOpenStyleBaseTempo()
{
    return currentStyleComponent ? currentStyleComponent->getBaseTempo() : ArrangerStyleDefaults::tempoBpm;
}

void Display::commitOpenStyleTempo(double tempo)
{
    if (currentStyleComponent)
        currentStyleComponent->commitTempo(tempo);
}
