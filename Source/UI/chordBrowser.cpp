/*
  ==============================================================================

    chordBrowser.cpp
    Created: 22 Feb 2026 9:52:13pm
    Author:  Kisuke

  ==============================================================================
*/

#include "chordBrowser.h"
#include "Common/AppColours.h"
#include "UI/Stage/StageFonts.h"

namespace
{
    namespace ChordPageText
    {
        const char* const searchHintUtf8      = "Search chords\xe2\x80\xa6";                         // "Search chords…"
        const char* const nameNotesSeparator  = " \xc2\xb7 ";                                       // " · "
        const char* const voicingSeparator    = " \xe2\x80\x93 ";                                   // " – "
        const char* const hintUtf8            = "Click a chord to see it \xc2\xb7 the keys show where to put your fingers";
        const juce::String rowNotesSeparator  = " ";
        const juce::String voicingTitles[]    = { "Root position", "1st inversion", "2nd inversion" };
    }

    constexpr bool  mediumFace     = false;   // silkscreen Medium (the board's .silk-dim)
    constexpr float noLetterSpacing = 0.0f;   // the voicing notes: mono, letter-spacing 0
    constexpr int   whiteEdgeWidth = 1;       // .mw inset -1px 0 0
}

//==============================================================================
struct ChordBrowserComponent::Rows : public juce::ListBoxModel
{
    explicit Rows (ChordBrowserComponent& ownerBrowser) : owner (ownerBrowser) {}

    int getNumRows() override { return owner.getNumShown(); }

    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool) override
    {
        if (! juce::isPositiveAndBelow (rowNumber, owner.filteredIndices.size()))
            return;
        const auto chordIndex = owner.filteredIndices[static_cast<size_t> (rowNumber)];
        const auto& chord = owner.allChords[chordIndex];
        const auto notes = ChordHelperModel::noteNames (ChordHelper::getNotesForChord (chord), ChordPageText::rowNotesSeparator);
        StageLcd::paintRow (g, { width, height }, owner.scale, chord.name, notes, chordIndex == owner.chosen);
    }

    void listBoxItemClicked (int rowNumber, const juce::MouseEvent&) override
    {
        owner.chooseShownRow (rowNumber);
    }

    ChordBrowserComponent& owner;
};

//==============================================================================
ChordBrowserComponent::ChordBrowserComponent (const std::vector<Chord>& chords)
    : allChords (chords),
      search (juce::String::fromUTF8 (ChordPageText::searchHintUtf8)),
      rows (std::make_unique<Rows> (*this))
{
    addAndMakeVisible (search);
    search.onTextChange = [this] { rebuildFilteredList(); };
    search.onEscapeKey  = [this] { if (onEscape) onEscape(); };

    listBox.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    listBox.setOutlineThickness (int {});
    listBox.getViewport()->setScrollBarsShown (false, false, true, false);   // scrolls by wheel, as the LCD lists
    listBox.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    listBox.setModel (rows.get());
    addAndMakeVisible (listBox);

    setScale (scale);
    rebuildFilteredList();
}

ChordBrowserComponent::~ChordBrowserComponent()
{
    listBox.setModel (nullptr);
}

void ChordBrowserComponent::setScale (float newScale)
{
    scale = newScale;
    search.setScale (scale);
    listBox.setRowHeight (juce::roundToInt (static_cast<float> (ChordHelperMetrics::chordRowHeight) * scale));
    resized();
    repaint();
}

void ChordBrowserComponent::setSearchText (const juce::String& text)
{
    search.setText (text, juce::dontSendNotification);
    rebuildFilteredList();
}

int ChordBrowserComponent::getNumShown() const
{
    return static_cast<int> (filteredIndices.size());
}

void ChordBrowserComponent::chooseShownRow (int row)
{
    if (! juce::isPositiveAndBelow (row, filteredIndices.size()))
        return;
    chosen = filteredIndices[static_cast<size_t> (row)];
    listBox.repaint();
    repaint();
    if (onChordChosen)
        onChordChosen (allChords[chosen]);
}

juce::String ChordBrowserComponent::getChosenName() const
{
    return chosen < allChords.size() ? allChords[chosen].name : juce::String();
}

void ChordBrowserComponent::rebuildFilteredList()
{
    const auto query = search.getText().trim();
    filteredIndices.clear();
    for (size_t chordIndex {}; chordIndex < allChords.size(); ++chordIndex)
        if (query.isEmpty() || allChords[chordIndex].name.containsIgnoreCase (query))
            filteredIndices.push_back (chordIndex);
    listBox.updateContent();
    listBox.repaint();
}

ChordHelperRects ChordBrowserComponent::layout() const
{
    return chordHelperLayout (getLocalBounds(), scale);
}

void ChordBrowserComponent::resized()
{
    const auto rects = layout();
    const auto padding = juce::roundToInt (static_cast<float> (StageMetrics::lcdListPadding) * scale);
    search.setBounds (rects.search);
    listBox.setBounds (rects.list.reduced (int {}, padding));
}

void ChordBrowserComponent::paint (juce::Graphics& g)
{
    using namespace ChordHelperMetrics;
    const auto rects = layout();
    StageLcd::paintPanel (g, rects.list.toFloat(), scale);

    if (chosen < allChords.size())
    {
        const auto& chord = allChords[chosen];

        // Name row: the name large, its notes after it on the same baseline.
        const auto nameFont = StageFonts::lcd (nameTextSize * scale);
        g.setColour (StagePalette::chordName);
        g.setFont (nameFont);
        g.drawText (chord.name, rects.name, juce::Justification::centredLeft, false);

        const auto notesX = static_cast<float> (rects.name.getX()) + juce::GlyphArrangement::getStringWidth (nameFont, chord.name)
                          + static_cast<float> (nameNotesGap) * scale;
        const juce::Rectangle<float> notesArea (notesX, static_cast<float> (rects.name.getY()) + static_cast<float> (nameNotesTop) * scale,
                                                static_cast<float> (rects.name.getRight()) - notesX, static_cast<float> (nameNotesHeight) * scale);
        g.setColour (StagePalette::silkscreenDimText);
        g.setFont (StageFonts::silkscreen (nameNotesTextSize * scale, mediumFace, StageMetrics::silkDimSpacing));
        g.drawText (ChordHelperModel::noteNames (ChordHelper::getNotesForChord (chord), juce::String::fromUTF8 (ChordPageText::nameNotesSeparator)),
                    notesArea, juce::Justification::centredLeft, false);

        const auto shapes = ChordHelperModel::voicings (chord);
        for (size_t column {}; column < shapes.size(); ++column)
            paintVoicing (g, rects, column, ChordPageText::voicingTitles[column], shapes[column]);
    }

    g.setColour (StagePalette::silkscreenDimText);
    g.setFont (StageFonts::silkscreen (StageMetrics::silkLabelSize * scale, mediumFace, StageMetrics::silkDimSpacing));
    g.drawText (juce::String::fromUTF8 (ChordPageText::hintUtf8).toUpperCase(), rects.hint, juce::Justification::centredLeft, false);
}

void ChordBrowserComponent::paintVoicing (juce::Graphics& g, const ChordHelperRects& rects, size_t column,
                                          const juce::String& title, const std::vector<int>& voicing) const
{
    using namespace ChordHelperMetrics;
    StageLcd::paintHeading (g, rects.voicingTitle[column].toFloat(), scale, title, {});

    // The board's .mini: a dark bed, three octaves of keys from its left edge, cut off at its right (overflow hidden).
    const auto bed = rects.keyboard[column].toFloat();
    g.setColour (StagePalette::miniKeyBed);
    g.fillRoundedRectangle (bed, static_cast<float> (keyboardCorner) * scale);
    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (rects.keyboard[column]);
        const auto isLit = [&voicing] (int note) { return std::find (voicing.begin(), voicing.end(), note) != voicing.end(); };
        for (const auto& key : MiniKeyboard::keys (scale))
        {
            const auto box = key.box.translated (bed.getX(), bed.getY());
            const bool lit = isLit (key.note);
            g.setColour (key.black ? (lit ? StagePalette::miniBlackLit : StagePalette::miniBlack)
                                   : (lit ? StagePalette::miniWhiteLit : StagePalette::miniWhite));
            g.fillRect (box);
            if (! key.black)
            {
                const auto edge = static_cast<float> (whiteEdgeWidth) * scale;
                g.setColour (StagePalette::miniWhiteEdge);
                g.fillRect (box.withLeft (box.getRight() - edge));
            }
        }
    }

    g.setColour (StagePalette::silkscreenDimText);
    g.setFont (StageFonts::lcd (voicingNotesTextSize * scale).withExtraKerningFactor (noLetterSpacing));
    g.drawText (ChordHelperModel::noteNames (voicing, juce::String::fromUTF8 (ChordPageText::voicingSeparator)),
                rects.voicingNotes[column], juce::Justification::centredLeft, false);
}
