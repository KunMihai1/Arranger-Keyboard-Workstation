#pragma once

#include <juce_graphics/juce_graphics.h>
#include <array>

/*  The panel finishes of board SkinsFinish (redesign B4): the matte "Stage" default and five optional surfaces.
    A finish paints the panel surface (and the key slip) and two side strips at the panel's ends; controls, screen
    and keys keep their own materials. Rendering is slow (fractal noise per pixel): draw once per finish and size into
    an image (renderSurface / renderSide) and keep it. */
namespace PanelFinish
{
    enum class Id { stage, rossoBurst, champagne, tobaccoBurst, midnightFlake, concertBlack };

    inline constexpr std::array<Id, 6> all { Id::stage, Id::rossoBurst, Id::champagne, Id::tobaccoBurst,
                                             Id::midnightFlake, Id::concertBlack };

    /** The settings file's text for a finish (PanelFinish key); persisted, never changes. */
    juce::String storedId (Id finish);

    /** The finish a stored text names; anything else (missing, old, typo) is the default Stage. */
    Id fromStored (const juce::String& stored);

    /** The short name on the Settings page's Finish buttons. */
    juce::String label (Id finish);

    /** The silkscreen ink for a label printed on this background: the usual light ink, or dark ink where it reads
        better (Champagne; the bright amber centre of Tobacco), whichever contrasts more. */
    juce::Colour inkFor (juce::Colour background, bool bright);

    /** A label's ink: its column's ink (one ink per printed column), unless that reads under 3:1 on the label's own
        background (the bright middle of a sunburst), then the ink that reads better there. */
    juce::Colour inkFor (juce::Colour columnBackground, juce::Colour labelBackground, bool bright);

    /** WCAG contrast ratio of two colours. */
    float contrastRatio (juce::Colour first, juce::Colour second);

    /** The average colour of an image over an area (the background a label sits on; noise averages out). */
    juce::Colour averageColour (const juce::Image& image, juce::Rectangle<int> area);

    /** The panel's three printed columns (sections | screen and its buttons | transport and Quick FX) inside the
        panel's content area: each prints in one ink, chosen from its average background, like real printed gear. */
    inline constexpr int inkColumnCount = 3;
    std::array<juce::Rectangle<int>, inkColumnCount> inkColumns (juce::Rectangle<int> content, float scale);

    /** Which of the ink columns a label at this x belongs to. */
    size_t inkColumnAt (const std::array<juce::Rectangle<int>, inkColumnCount>& columns, int labelCentreX);

    /** Width of each side strip at this UI scale. */
    float sideWidth (float scale);

    /** The finish's surface for a box of this size at this UI scale; an invalid image for Stage. The panel has the
        trims (Midnight's chrome, Concert's pinstripes); the key slip is the bare surface. */
    enum class Part { panel, keySlip };
    juce::Image renderSurface (Id finish, int width, int height, float scale, Part part = Part::panel);

    /** One side strip (left or right end of the panel); an invalid image for Stage. */
    juce::Image renderSide (Id finish, int width, int height, float scale, bool isRightSide);
}
