#include "StagePanel.h"
#include "StageFonts.h"
#include "../../Common/AppColours.h"
#include "../../Styles/SectionGroupComponent.h"

namespace
{
    // Printed labels where they differ from the component name (board StageRevised).
    const juce::String endingLabelPrefix = "End ";
    const char* const  tempoDownUtf8     = "Tempo \xe2\x88\x92";   // "Tempo" + U+2212 minus sign
    const juce::String tempoUpLabel      = "Tempo +";
    const juce::String syncStartLabel    = "Sync\nStart";
    const juce::String quickFxTitle      = "QUICK FX";
    const juce::String partTitle         = "PART";

    constexpr int introCount     = 3;
    constexpr int endingCount    = 3;
    constexpr int variationCount = 4;
    constexpr int fillCount      = 4;
    constexpr int firstNumber    = 1;
    constexpr int autoFillCount  = 1;   // Auto Fill ends the fill row

    constexpr int transportFirstRow  = 2;   // Rec, File
    constexpr float halves           = 2.0f;
    constexpr bool semiBold          = true;
    constexpr bool medium            = false;
    constexpr int finishSettleMilliseconds = 150;   // redraw the finish once a window drag has paused this long

    /** Width a silkscreen text needs, as CSS lays it out: letter spacing follows the last letter too, and
        rounding up keeps drawText from dropping the last glyph. */
    // The measured width already holds the letter-spacing after every letter, as CSS counts it.
    float silkWidth (const juce::Font& font, const juce::String& text)
    {
        return std::ceil (juce::GlyphArrangement::getStringWidth (font, text));
    }

    juce::String numbered (const juce::String& prefix, int number)
    {
        return prefix + juce::String (number);
    }
}

StagePanel::StagePanel (double styleTempo)
    : stageScreen (styleTempo)
{
    // The panel fills its whole area, so it is opaque: a knob or LED repaint then never makes JUCE
    // redraw the window's background image behind it.
    setOpaque (true);

    using Tone = RubberButton::LabelTone;
    using Led  = RubberButton::LedColour;

    // Left: section buttons, row by row; each reports the arranger's name for it.
    auto addSection = [this] (const juce::String& sectionName, const juce::String& label)
    {
        auto& button = addKey (sectionName, label, Led::red, Tone::dim);
        button.onClick = [this, sectionName] { if (onSection != nullptr) onSection (sectionName); };
        sectionButtons.push_back (&button);
    };

    for (int number = firstNumber; number <= introCount; ++number)
        addSection (numbered (SectionGroupNames::introPrefix, number), numbered (SectionGroupNames::introPrefix, number));
    for (int number = firstNumber; number <= endingCount; ++number)
        addSection (numbered (SectionGroupNames::endingPrefix, number), numbered (endingLabelPrefix, number));
    sectionRowLengths.push_back (introCount + endingCount);

    for (int number = firstNumber; number <= variationCount; ++number)
        addSection (numbered (SectionGroupNames::variationPrefix, number), numbered (SectionGroupNames::variationPrefix, number));
    addSection (SectionGroupNames::breakButton, SectionGroupNames::breakButton);
    sectionRowLengths.push_back (static_cast<int> (sectionButtons.size()) - sectionRowLengths.front());

    for (int number = firstNumber; number <= fillCount; ++number)
        addSection (numbered (SectionGroupNames::fillPrefix, number), numbered (SectionGroupNames::fillPrefix, number));
    autoFillButton = &addKey (StagePanelNames::autoFill, StagePanelNames::autoFill, Led::green, Tone::dim);
    autoFillButton->onClick = [this] { if (onAutoFill != nullptr) onAutoFill(); };
    sectionRowLengths.push_back (fillCount + autoFillCount);

    // Right: transport grid.
    recButton       = &addKey (StagePanelNames::rec, StagePanelNames::rec, Led::red, Tone::bright);
    fileButton      = &addKey (StagePanelNames::file, StagePanelNames::file, Led::red, Tone::bright);
    syncStartButton = &addKey (StagePanelNames::syncStart, syncStartLabel, Led::green, Tone::bright);
    auto& stopButton = addKey (StagePanelNames::stop, StagePanelNames::stop, Led::red, Tone::bright);
    startButton     = &addKey (StagePanelNames::start, StagePanelNames::start, Led::green, Tone::bright);
    transportButtons = { recButton, fileButton, syncStartButton, &stopButton, startButton };
    recButton->onClick       = [this] { if (onRec != nullptr) onRec(); };
    fileButton->onClick      = [this] { if (onFile != nullptr) onFile(); };
    syncStartButton->onClick = [this] { if (onSyncStart != nullptr) onSyncStart(); };
    stopButton.onClick       = [this] { if (onStop != nullptr) onStop(); };
    startButton->onClick     = [this] { if (onStart != nullptr) onStart(); };
    pageButtons.emplace_back (fileButton, StagePage::file);

    // Centre: page buttons and the tempo buttons beside the screen.
    auto addPage = [this] (const juce::String& pageName, StagePage page, RubberButton::LabelSide side)
    {
        auto& button = addScreenButton (pageName, pageName, side);
        button.onClick = [this, page] { if (onPage != nullptr) onPage (page); };
        pageButtons.emplace_back (&button, page);
        return &button;
    };

    using Side = RubberButton::LabelSide;
    leftScreenButtons  = { addPage (StagePanelNames::home, StagePage::home, Side::left),
                           addPage (StagePanelNames::styles, StagePage::styles, Side::left),
                           addPage (StagePanelNames::voices, StagePage::voices, Side::left),
                           &addScreenButton (StagePanelNames::tempoDown, juce::String::fromUTF8 (tempoDownUtf8), Side::left) };
    rightScreenButtons = { addPage (StagePanelNames::effects, StagePage::effects, Side::right),
                           addPage (StagePanelNames::settings, StagePage::settings, Side::right),
                           addPage (StagePanelNames::chords, StagePage::chords, Side::right),
                           &addScreenButton (StagePanelNames::tempoUp, tempoUpLabel, Side::right) };

    auto holdOnPress = [this] (RubberButton* button, int direction)
    {
        button->onStateChange = [this, button, direction]
        {
            if (button->getState() == juce::Button::buttonDown)
                startTempoHold (direction);
            else if (holdDirection == direction)
                stopTempoHold();
        };
    };
    holdOnPress (leftScreenButtons.back(), StageMetrics::tempoStepDown);
    holdOnPress (rightScreenButtons.back(), StageMetrics::tempoStepUp);

    addAndMakeVisible (stageScreen);

    // Right: Quick FX knobs and the part they act on.
    for (auto* knob : { &volumeKnob, &reverbKnob })
        addAndMakeVisible (*knob);
    volumeKnob.onValueChange = [this] { if (onQuickKnob != nullptr) onQuickKnob (QuickKnob::volume, juce::roundToInt (volumeKnob.getValue())); };
    reverbKnob.onValueChange = [this] { if (onQuickKnob != nullptr) onQuickKnob (QuickKnob::reverb, juce::roundToInt (reverbKnob.getValue())); };
    volumeKnob.onDragEnd = [this] { if (onQuickKnobReleased != nullptr) onQuickKnobReleased (QuickKnob::volume, juce::roundToInt (volumeKnob.getValue())); };
    reverbKnob.onDragEnd = [this] { if (onQuickKnobReleased != nullptr) onQuickKnobReleased (QuickKnob::reverb, juce::roundToInt (reverbKnob.getValue())); };

    auto addPartToggle = [this] (const juce::String& partName, bool left)
    {
        auto* toggle = buttons.add (std::make_unique<RubberButton> (partName, RubberButton::Style::ledToggle, Led::red));
        toggle->onClick = [this, left] { if (onPart != nullptr) onPart (left); };
        addAndMakeVisible (*toggle);
        return toggle;
    };
    partLeftButton  = addPartToggle (StagePanelNames::partLeft, true);
    partRightButton = addPartToggle (StagePanelNames::partRight, false);
}

StagePanel::~StagePanel()
{
    stopTimer();
}

RubberButton& StagePanel::addKey (const juce::String& buttonName, const juce::String& label,
                                  RubberButton::LedColour colour, RubberButton::LabelTone tone)
{
    auto* button = buttons.add (std::make_unique<RubberButton> (label, RubberButton::Style::key, colour));
    button->setName (buttonName);
    button->setLabelTone (tone);
    addAndMakeVisible (*button);
    return *button;
}

RubberButton& StagePanel::addScreenButton (const juce::String& buttonName, const juce::String& label, RubberButton::LabelSide side)
{
    auto* button = buttons.add (std::make_unique<RubberButton> (label, RubberButton::Style::screen, RubberButton::LedColour::red));
    button->setName (buttonName);
    button->setLabelSide (side);
    addAndMakeVisible (*button);
    return *button;
}

float StagePanel::scaled (float referenceLength) const { return referenceLength * scale; }
float StagePanel::scaled (int referenceLength) const   { return scaled (static_cast<float> (referenceLength)); }
StageScreen& StagePanel::screen()                      { return stageScreen; }
bool StagePanel::isHoldingTempo() const                { return holdDirection != int {}; }

void StagePanel::setScale (float newScale)
{
    scale = newScale;
    for (auto* button : buttons)
        button->setScale (scale);
    volumeKnob.setScale (scale);
    reverbKnob.setScale (scale);
    stageScreen.setScale (scale);
    resized();
    repaint();
}

//==============================================================================
void StagePanel::setSectionLit (const juce::String& sectionName)
{
    for (auto* button : sectionButtons)
        button->setLedOn (sectionName.isNotEmpty() && button->getName() == sectionName);
}

void StagePanel::setAutoFill (bool on)  { autoFillButton->setLedOn (on); }
void StagePanel::setSyncStart (bool on) { syncStartButton->setLedOn (on); }
void StagePanel::setRecording (bool on) { recButton->setLedOn (on); }
void StagePanel::setPlaying (bool on)   { startButton->setLedOn (on); }

void StagePanel::setPageLed (StagePage openPage)
{
    for (const auto& [button, page] : pageButtons)
        button->setLedOn (openPage != StagePage::none && page == openPage);
}

void StagePanel::setPartLeds (bool leftOn, bool rightOn)
{
    partLeftButton->setLedOn (leftOn);
    partRightButton->setLedOn (rightOn);
}

void StagePanel::setQuickKnobValues (int volumeValue, int reverbValue)
{
    shownVolume = volumeValue;
    shownReverb = reverbValue;
    revertQuickKnobs();
}

void StagePanel::revertQuickKnobs()
{
    volumeKnob.setValue (shownVolume, juce::dontSendNotification);
    reverbKnob.setValue (shownReverb, juce::dontSendNotification);
}

//==============================================================================
void StagePanel::startTempoHold (int direction)
{
    stopTempoHold();
    holdDirection = direction;
    holdStepIndex = {};
    stageScreen.beginTempoGesture();   // the style hears the tempo once, on release
    stageScreen.stepTempo (direction);
    if (stageScreen.tempo().isLocked())
    {
        stopTempoHold();
        return;
    }
    startTimer (TempoModel::holdDelayMs (holdStepIndex));
}

void StagePanel::stopTempoHold()
{
    if (holdDirection != int {})
        stageScreen.endTempoGesture();
    holdDirection = {};
    stopTimer();
}

void StagePanel::timerCallback()
{
    if (holdDirection == int {} || stageScreen.tempo().isLocked())
    {
        stopTempoHold();
        return;
    }

    ++holdStepIndex;
    stageScreen.stepTempo (holdDirection);
    startTimer (TempoModel::holdDelayMs (holdStepIndex));
}

//==============================================================================
juce::Rectangle<int> StagePanel::getContentBounds() const
{
    const auto bounds = getLocalBounds();
    const auto contentWidth = juce::jmin (bounds.getWidth(), juce::roundToInt (scaled (StageMetrics::referenceWidth)));
    return bounds.withSizeKeepingCentre (contentWidth, bounds.getHeight());
}

void StagePanel::resized()
{
    using namespace StageMetrics;
    // A finish drawn for another size is stretched until the resize settles, then drawn again.
    if (finish != PanelFinish::Id::stage && finishSurface.isValid()
        && (finishSurface.getBounds() != getLocalBounds() || ! juce::approximatelyEqual (finishScale, scale)))
        finishRefresh.startTimer (finishSettleMilliseconds);
    const auto content = getContentBounds().toFloat();
    const auto innerTop    = content.getY() + scaled (panelPaddingVertical);
    const auto innerHeight = scaled (panelHeight - panelPaddingVertical - panelPaddingVertical);
    auto centredTop = [innerTop, innerHeight] (float blockHeight) { return innerTop + (innerHeight - blockHeight) / halves; };

    // A key button's component is centred on its cap; its cell starts with the LED.
    const auto capCentreInCell = scaled (ledDiameter + ledToButtonGap) + scaled (buttonWidth) / halves;
    auto placeKey = [capCentreInCell] (RubberButton& button, float cellLeft, float top)
    {
        const auto width = button.getPreferredWidth();
        button.setBounds (button.boundsForBoardBox ({ juce::roundToInt (cellLeft + capCentreInCell - static_cast<float> (width) / halves),
                                                      juce::roundToInt (top), width, button.getPreferredHeight() }));
    };

    // --- Left third: section rows on a 6-column grid ---
    {
        const auto left = content.getX() + scaled (panelPaddingHorizontal);
        const auto columnWidth = scaled (panelSideColumnWidth) / static_cast<float> (sectionGridColumns);
        const auto rowHeight = static_cast<float> (sectionButtons.front()->getPreferredHeight());
        const auto rowCount = static_cast<float> (sectionRowLengths.size());
        const auto blockHeight = (rowHeight + scaled (sectionRowGap)) * rowCount - scaled (sectionRowGap);   // rows and the gaps between them
        auto top = centredTop (blockHeight);

        size_t buttonIndex {};
        std::vector<RubberButton*> rowOrder (sectionButtons);
        rowOrder.push_back (autoFillButton);
        for (const auto rowLength : sectionRowLengths)
        {
            for (int columnIndex {}; columnIndex < rowLength; ++columnIndex)
                placeKey (*rowOrder[buttonIndex++], left + columnWidth * static_cast<float> (columnIndex), top);
            top += rowHeight + scaled (sectionRowGap);
        }
    }

    // --- Right third: transport grid | rule | Quick FX, packed to the right ---
    const auto rightEdge = content.getX() + scaled (panelPaddingHorizontal + panelSideColumnWidth + panelCentreColumnWidth + panelSideColumnWidth);
    const auto transportWidth = scaled (transportColumns * transportColumnWidth);
    const auto quickLeft = rightEdge - scaled (quickFxWidth);
    const auto ruleLeft  = quickLeft - scaled (rightThirdGap + ruleWidth);
    const auto transportLeft = ruleLeft - scaled (rightThirdGap) - transportWidth;
    {
        const auto firstRowHeight  = static_cast<float> (recButton->getPreferredHeight());
        const auto secondRowHeight = static_cast<float> (syncStartButton->getPreferredHeight());
        const auto top = centredTop (firstRowHeight + scaled (transportRowGap) + secondRowHeight);
        for (size_t transportIndex {}; transportIndex < transportButtons.size(); ++transportIndex)
        {
            const auto inFirstRow = transportIndex < static_cast<size_t> (transportFirstRow);
            const auto column = inFirstRow ? transportIndex : transportIndex - static_cast<size_t> (transportFirstRow);
            placeKey (*transportButtons[transportIndex], transportLeft + scaled (transportColumnWidth) * static_cast<float> (column),
                      inFirstRow ? top : top + firstRowHeight + scaled (transportRowGap));
        }
        rightRule = { ruleLeft, centredTop (scaled (rightRuleHeight)), scaled (ruleWidth), scaled (rightRuleHeight) };
    }
    {
        const auto titleFont = StageFonts::silkscreen (scaled (silkLabelSize), semiBold, silkLabelSpacing);
        const auto titleHeight = scaled (silkLabelLineHeight);
        const auto knobHeight  = static_cast<float> (volumeKnob.getPreferredHeight());
        const auto partHeight  = static_cast<float> (partLeftButton->getPreferredHeight());
        auto top = centredTop (titleHeight + scaled (quickFxRowGap) + knobHeight + scaled (quickFxRowGap) + partHeight);

        const auto titleWidth = silkWidth (titleFont, quickFxTitle);
        quickFxTitleArea = { quickLeft, top, titleWidth, titleHeight };
        const auto ruleStart = quickLeft + titleWidth + scaled (quickFxTitleGap);
        quickFxRule = { ruleStart, top + (titleHeight - scaled (ruleWidth)) / halves, rightEdge - ruleStart, scaled (ruleWidth) };
        top += titleHeight + scaled (quickFxRowGap);

        volumeKnob.setBounds (juce::roundToInt (quickLeft), juce::roundToInt (top), volumeKnob.getPreferredWidth(), volumeKnob.getPreferredHeight());
        reverbKnob.setBounds (juce::roundToInt (rightEdge) - reverbKnob.getPreferredWidth(), juce::roundToInt (top),
                              reverbKnob.getPreferredWidth(), reverbKnob.getPreferredHeight());
        top += knobHeight + scaled (quickFxRowGap);

        const auto partFont = StageFonts::silkscreen (scaled (silkLabelSize), medium, partLabelSpacing);
        const auto partWidth = silkWidth (partFont, partTitle);
        partLabelArea = { quickLeft, top, partWidth, partHeight };
        auto toggleLeft = quickLeft + partWidth + scaled (partGap);
        for (auto* toggle : { partLeftButton, partRightButton })
        {
            toggle->setBounds (toggle->boundsForBoardBox ({ juce::roundToInt (toggleLeft), juce::roundToInt (top), toggle->getPreferredWidth(), toggle->getPreferredHeight() }));
            toggleLeft += static_cast<float> (toggle->getPreferredWidth()) + scaled (partGap);
        }
    }

    // --- Centre: screen buttons | screen | screen buttons, centred in the 600 column ---
    {
        auto widestOf = [] (const std::vector<RubberButton*>& column)
        {
            int widest {};
            for (auto* button : column)
                widest = juce::jmax (widest, button->getPreferredWidth());
            return static_cast<float> (widest);
        };
        const auto leftWidth  = widestOf (leftScreenButtons);
        const auto rightWidth = widestOf (rightScreenButtons);
        const auto screenWidthPx = static_cast<float> (stageScreen.getPreferredWidth());
        const auto total = leftWidth + scaled (centreColumnGap) + screenWidthPx + scaled (centreColumnGap) + rightWidth;
        const auto centreLeft = content.getX() + scaled (panelPaddingHorizontal + panelSideColumnWidth);
        const auto startX = centreLeft + (scaled (panelCentreColumnWidth) - total) / halves;

        const auto rowHeight = static_cast<float> (leftScreenButtons.front()->getPreferredHeight());
        const auto rows = static_cast<float> (screenButtonsPerSide);
        const auto columnTop = centredTop ((rowHeight + scaled (screenButtonRowGap)) * rows - scaled (screenButtonRowGap));
        for (size_t rowIndex {}; rowIndex < leftScreenButtons.size(); ++rowIndex)
        {
            const auto top = juce::roundToInt (columnTop + (rowHeight + scaled (screenButtonRowGap)) * static_cast<float> (rowIndex));
            auto* leftButton = leftScreenButtons[rowIndex];
            leftButton->setBounds (leftButton->boundsForBoardBox ({ juce::roundToInt (startX + leftWidth) - leftButton->getPreferredWidth(), top,
                                                                    leftButton->getPreferredWidth(), leftButton->getPreferredHeight() }));
            auto* rightButton = rightScreenButtons[rowIndex];
            rightButton->setBounds (rightButton->boundsForBoardBox ({ juce::roundToInt (startX + leftWidth + scaled (centreColumnGap) + screenWidthPx + scaled (centreColumnGap)), top,
                                                                      rightButton->getPreferredWidth(), rightButton->getPreferredHeight() }));
        }

        stageScreen.setBounds (juce::roundToInt (startX + leftWidth + scaled (centreColumnGap)),
                               juce::roundToInt (centredTop (scaled (screenHeight))),
                               stageScreen.getPreferredWidth(), stageScreen.getPreferredHeight());
    }
}

void StagePanel::setFinish (PanelFinish::Id newFinish)
{
    if (newFinish == finish)
        return;
    finish = newFinish;
    renderFinish();
    repaint();
}

juce::Colour StagePanel::silkscreenInk (const juce::Component& widget, bool bright) const
{
    return inkOver (getLocalArea (&widget, widget.getLocalBounds()), bright);
}

juce::Colour StagePanel::inkOver (juce::Rectangle<int> area, bool bright) const
{
    if (finish == PanelFinish::Id::stage || ! finishSurface.isValid())
        return bright ? StagePalette::silkscreenText : StagePalette::silkscreenDimText;

    // One ink per printed column, from the finish under the whole column (labels in a row never switch ink).
    const auto column = PanelFinish::inkColumnAt (PanelFinish::inkColumns (getContentBounds(), scale), area.getCentreX());
    return PanelFinish::inkFor (columnBackgrounds[column], PanelFinish::averageColour (finishSurface, area), bright);
}

void StagePanel::renderFinish()
{
    const auto sideWidth = juce::roundToInt (PanelFinish::sideWidth (scale));
    finishSurface   = PanelFinish::renderSurface (finish, getWidth(), getHeight(), scale);
    finishLeftSide  = PanelFinish::renderSide (finish, sideWidth, getHeight(), scale, false);
    finishRightSide = PanelFinish::renderSide (finish, sideWidth, getHeight(), scale, true);
    finishScale = scale;

    const auto columns = PanelFinish::inkColumns (getContentBounds(), scale);
    for (size_t column {}; column < columns.size(); ++column)
        columnBackgrounds[column] = PanelFinish::averageColour (finishSurface, columns[column]);
}

void StagePanel::paint (juce::Graphics& g)
{
    using namespace StageMetrics;
    const auto bounds = getLocalBounds().toFloat();
    if (finish == PanelFinish::Id::stage)
    {
        g.setGradientFill (juce::ColourGradient::vertical (StagePalette::panelTop, bounds.getY(), StagePalette::panelBottom, bounds.getBottom()));
        g.fillRect (bounds);
    }
    else
    {
        if (! finishSurface.isValid())
            renderFinish();
        const auto sideWidth = PanelFinish::sideWidth (scale);
        g.drawImage (finishSurface, bounds);
        g.drawImage (finishLeftSide, bounds.withWidth (sideWidth));
        g.drawImage (finishRightSide, bounds.withLeft (bounds.getRight() - sideWidth));
    }
    g.setColour (StagePalette::panelBottomEdge);
    g.fillRect (bounds.withTop (bounds.getBottom() - scaled (panelBottomEdge)));

    g.setColour (StagePalette::ruleLine);
    g.fillRect (rightRule);
    g.fillRect (quickFxRule);

    // Text is laid out even where it is clipped away, so only draw it when it is being repainted
    // (a knob turn repaints just the knob's area).
    if (g.clipRegionIntersects (quickFxTitleArea.getSmallestIntegerContainer()))
    {
        g.setColour (inkOver (quickFxTitleArea.getSmallestIntegerContainer(), true));
        g.setFont (StageFonts::silkscreen (scaled (silkLabelSize), semiBold, silkLabelSpacing));
        g.drawText (quickFxTitle, quickFxTitleArea, juce::Justification::centredLeft, false);
    }

    if (g.clipRegionIntersects (partLabelArea.getSmallestIntegerContainer()))
    {
        g.setColour (inkOver (partLabelArea.getSmallestIntegerContainer(), false));
        g.setFont (StageFonts::silkscreen (scaled (silkLabelSize), medium, partLabelSpacing));
        g.drawText (partTitle, partLabelArea, juce::Justification::centredLeft, false);
    }
}
