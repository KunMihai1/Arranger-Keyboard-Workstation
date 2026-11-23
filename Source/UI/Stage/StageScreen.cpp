#include "StageScreen.h"
#include "StageFonts.h"
#include "../../Common/AppColours.h"
#include "../../Common/AppInfo.h"

namespace
{
    // Text on the screen (board StageRevised).
    const juce::String styleTopLine      = "STYLE";
    const juce::String tempoWord         = "TEMPO";
    const char* const  middleDotUtf8     = " \xc2\xb7 ";   // " · " between time signature and TEMPO
    const juce::String barPrefix         = "BAR ";
    const juce::String backToStylePrefix = "TEMPO BACK TO STYLE: ";
    const juce::String lockedMessage     = "TEMPO LOCKED WHILE PLAYING";
    const juce::String digits            = "0123456789";
    const juce::String zeroDigit         = "0";
    constexpr int barDigits      = 2;
    constexpr int maxTempoDigits = 3;

    // Drawing details of the board's CSS, reference px / alpha.
    constexpr int   bezelHighlightHeight = 1;       // 0 1px 0 rgba(255,255,255,.06) under the bezel
    constexpr float bezelHighlightAlpha  = 0.06f;
    constexpr float bezelShadeHeight     = 4.0f;    // inset 0 2px 4px rgba(0,0,0,.8)
    constexpr float bezelShadeAlpha      = 0.8f;
    constexpr float lampShadeAlpha       = 0.7f;    // inset 0 1px 1px rgba(0,0,0,.7)
    constexpr float lampEdgeAlpha        = 0.05f;   // 0 1px 0 rgba(255,255,255,.05)
    constexpr float downbeatShineAlpha   = 0.35f;   // inset 0 1px 0 rgba(255,255,255,.35)
    constexpr float beatShineAlpha       = 0.45f;
    constexpr float hairline             = 1.0f;
    constexpr float tempoHoverAlpha      = 0.12f;   // .tempo:hover rgba(27,34,22,.12)
    constexpr float tempoSelectionAlpha  = 0.25f;   // the selected digits: a faint LCD tint, text stays light
    constexpr int   messageRefreshMs     = 100;
    constexpr int   scrollRefreshMs      = 30;      // smooth scrolling of a long message
    constexpr float halves               = 2.0f;
}

StageScreen::StageScreen (double styleTempo)
    : tempoModel (styleTempo),
      clock ([] { return juce::Time::getMillisecondCounterHiRes(); })
{
    addChildComponent (tempoEditor);
    tempoEditor.setInputRestrictions (maxTempoDigits, digits);
    tempoEditor.setJustification (juce::Justification::centredRight);
    tempoEditor.setSelectAllWhenFocused (true);
    // .tempo-input: light digits on a dark field with 2px corners. The field is painted by the
    // screen (a TextEditor background is square); the selection is only a tint, so the digits
    // stay light while select-all makes typing replace the tempo.
    tempoEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    tempoEditor.setColour (juce::TextEditor::textColourId, StagePalette::lcd);
    tempoEditor.setColour (juce::TextEditor::highlightColourId, StagePalette::lcd.withAlpha (tempoSelectionAlpha));
    tempoEditor.setColour (juce::TextEditor::highlightedTextColourId, StagePalette::lcd);
    tempoEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    tempoEditor.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    tempoEditor.setColour (juce::CaretComponent::caretColourId, StagePalette::lcd);
    tempoEditor.onReturnKey = [this] { closeTempoEditor (true); };
    tempoEditor.onEscapeKey = [this] { closeTempoEditor (false); };
    tempoEditor.onFocusLost = [this] { closeTempoEditor (true); };
}

StageScreen::~StageScreen()
{
    stopTimer();
}

float  StageScreen::scaled (float referenceLength) const { return referenceLength * scale; }
float  StageScreen::scaled (int referenceLength) const   { return scaled (static_cast<float> (referenceLength)); }
double StageScreen::now() const                          { return clock(); }
TempoModel& StageScreen::tempo()                         { return tempoModel; }
bool   StageScreen::isEditingTempo() const               { return editingTempo; }
juce::TextEditor& StageScreen::getTempoEditor()          { return tempoEditor; }
void   StageScreen::setClock (std::function<double()> newClock) { clock = std::move (newClock); }

void StageScreen::setScale (float newScale)
{
    scale = newScale;
    resized();
    repaint();
}

int StageScreen::getPreferredWidth() const
{
    return juce::roundToInt (scaled (StageMetrics::screenWidth));
}

int StageScreen::getPreferredHeight() const
{
    return juce::roundToInt (scaled (StageMetrics::screenHeight + bezelHighlightHeight));
}

void StageScreen::setContent (const Content& newContent)
{
    content = newContent;
    if (content.asleep && editingTempo)
        closeTempoEditor (false);
    repaint();
}

void StageScreen::postMessage (const juce::String& message)
{
    // A message sits where STYLE was (board); one too long for that takes the whole top row, and one still
    // too wide scrolls through and stays long enough to be read.
    const auto font  = StageFonts::lcd (scaled (StageMetrics::lcdTopRowSize));
    const auto width = juce::GlyphArrangement::getStringWidth (font, message.toUpperCase());
    messageOverflow  = juce::jmax (float {}, width - layout().lcdContent.getWidth());
    messages.post (message, now(), StageText::marqueeDurationMs (messageOverflow, scaled (StageMetrics::messageScrollSpeed)));
    startTimer (messageOverflow > float {} ? scrollRefreshMs : messageRefreshMs);
    repaint();
}

void StageScreen::timerCallback()
{
    repaint();
    if (messages.current (now()).isEmpty())
        stopTimer();
}

void StageScreen::setTempoLocked (bool shouldBeLocked)
{
    tempoModel.setLocked (shouldBeLocked);
    if (shouldBeLocked && editingTempo)
        closeTempoEditor (false);
}

void StageScreen::showTempo (double currentTempo, double styleTempo)
{
    tempoModel.setStyleTempo (styleTempo);
    if (! editingTempo && tempoGestures == int {})   // not while typing, or holding / dragging (not reported yet)
        tempoModel.showValue (currentTempo);
    repaint();
}

juce::String StageScreen::topRightText() const
{
    return content.timeSignature.isEmpty() ? tempoWord : content.timeSignature + juce::String::fromUTF8 (middleDotUtf8) + tempoWord;
}

bool StageScreen::isMessageShownAlone() const
{
    const auto message = messages.current (now());
    if (message.isEmpty())
        return false;

    const auto font = StageFonts::lcd (scaled (StageMetrics::lcdTopRowSize));
    const auto besideOthers = juce::GlyphArrangement::getStringWidth (font, message)
                            + juce::GlyphArrangement::getStringWidth (font, content.mode)
                            + juce::GlyphArrangement::getStringWidth (font, topRightText())
                            + scaled (StageMetrics::lcdTopRowMinGap) * halves;
    return besideOthers > layout().lcdContent.getWidth();
}

juce::String StageScreen::getTopLineText() const
{
    const auto message = messages.current (now());
    if (message.isNotEmpty())
        return message;

    return content.asleep ? juce::String (AppInfo::appName).toUpperCase() : styleTopLine;
}

bool StageScreen::tempoVisible() const
{
    return ! content.asleep;
}

juce::String StageScreen::tempoText() const
{
    return juce::String (juce::roundToInt (tempoModel.value()));
}

void StageScreen::reportTempoChange (bool changed)
{
    if (tempoModel.isLocked())
    {
        postMessage (lockedMessage);
        return;
    }

    if (changed)
    {
        repaint();
        if (tempoGestures > int {})
            tempoReportPending = true;
        else if (onTempoChanged != nullptr)
            onTempoChanged (tempoModel.value());
    }
}

void StageScreen::beginTempoGesture()
{
    ++tempoGestures;
}

void StageScreen::endTempoGesture()
{
    if (tempoGestures == int {})
        return;
    if (--tempoGestures > int {} || ! tempoReportPending)
        return;

    tempoReportPending = false;
    if (onTempoChanged != nullptr)
        onTempoChanged (tempoModel.value());
}

bool StageScreen::stepTempo (int direction)
{
    const auto changed = tempoModel.step (direction);
    reportTempoChange (changed);
    return changed;
}

//==============================================================================
StageScreen::Layout StageScreen::layout() const
{
    using namespace StageMetrics;
    Layout result;
    auto bounds = getLocalBounds().toFloat();
    result.bezel = bounds.withTrimmedBottom (scaled (bezelHighlightHeight));

    auto inner = result.bezel.withTrimmedTop (scaled (bezelPaddingTop))
                             .withTrimmedBottom (scaled (bezelPaddingBottom))
                             .reduced (scaled (bezelPaddingSide), float {});
    result.lampRow = inner.removeFromBottom (scaled (lampHeight)).reduced (scaled (lampRowInset), float {});
    inner.removeFromBottom (scaled (bezelGap));
    result.lcd = inner;
    result.lcdContent = inner.reduced (scaled (lcdPaddingSide), scaled (lcdPaddingVertical));

    // The three rows are spread like CSS justify-content: space-between.
    const auto topFont    = StageFonts::lcd (scaled (lcdTopRowSize));
    const auto titleFont  = StageFonts::lcd (scaled (lcdTitleSize));
    const auto bottomFont = StageFonts::lcd (scaled (lcdBottomRowSize));
    const auto rowArea    = result.lcdContent;
    const auto spare      = rowArea.getHeight() - topFont.getHeight() - titleFont.getHeight() - bottomFont.getHeight();
    const auto rowGap     = spare / halves;

    result.topBaseline    = rowArea.getY() + topFont.getAscent();
    result.titleBaseline  = rowArea.getY() + topFont.getHeight() + rowGap + titleFont.getAscent();
    result.bottomBaseline = rowArea.getBottom() - bottomFont.getHeight() + bottomFont.getAscent();
    return result;
}

juce::Rectangle<float> StageScreen::getTempoBounds() const
{
    if (! tempoVisible())
        return {};

    const auto screenLayout = layout();
    const auto font  = StageFonts::lcd (scaled (StageMetrics::lcdTempoSize));
    const auto width = juce::GlyphArrangement::getStringWidth (font, tempoText()) + scaled (StageMetrics::tempoPaddingSide) * halves;
    return { screenLayout.lcdContent.getRight() - width, screenLayout.titleBaseline - font.getAscent(), width, font.getHeight() };
}

void StageScreen::resized()
{
    if (! editingTempo)
        return;

    const auto tempoArea = getTempoBounds();
    const auto editorArea = juce::Rectangle<float> (scaled (StageMetrics::tempoEditorWidth), scaled (StageMetrics::tempoEditorHeight))
                                .withCentre ({ tempoArea.getRight() - scaled (StageMetrics::tempoEditorWidth) / halves, tempoArea.getCentreY() });
    tempoEditor.setBounds (editorArea.toNearestInt());
    tempoEditor.setFont (StageFonts::lcd (scaled (StageMetrics::tempoEditorSize)));
    tempoEditor.applyFontToAllText (StageFonts::lcd (scaled (StageMetrics::tempoEditorSize)));
    tempoEditor.setIndents (juce::roundToInt (scaled (StageMetrics::tempoEditorPadding)), int {});
}

//==============================================================================
void StageScreen::paint (juce::Graphics& g)
{
    using namespace StageMetrics;
    const auto screenLayout = layout();

    // Bezel: dark, with a soft shadow inside its top edge and a faint highlight under it.
    const auto bezelCorner = scaled (bezelCornerRadius);
    g.setColour (juce::Colours::white.withAlpha (bezelHighlightAlpha));
    g.fillRoundedRectangle (screenLayout.bezel.translated (float {}, scaled (bezelHighlightHeight)), bezelCorner);
    g.setColour (StagePalette::screenBezel);
    g.fillRoundedRectangle (screenLayout.bezel, bezelCorner);
    {
        juce::Graphics::ScopedSaveState saved (g);
        juce::Path bezelShape;
        bezelShape.addRoundedRectangle (screenLayout.bezel, bezelCorner);
        g.reduceClipRegion (bezelShape);
        g.setGradientFill (juce::ColourGradient::vertical (juce::Colours::black.withAlpha (bezelShadeAlpha), screenLayout.bezel.getY(),
                                                           juce::Colours::transparentBlack, screenLayout.bezel.getY() + scaled (bezelShadeHeight)));
        g.fillRect (screenLayout.bezel);
    }

    // LCD and its three rows of text.
    g.setColour (StagePalette::lcd);
    g.fillRoundedRectangle (screenLayout.lcd, scaled (lcdCornerRadius));
    g.setColour (StagePalette::lcdInk);
    const auto area = screenLayout.lcdContent;

    // Top row: a message alone (scrolling if too wide), else three items spread like space-between.
    const auto topFont = StageFonts::lcd (scaled (lcdTopRowSize));
    g.setFont (topFont);
    const auto topBaseline = juce::roundToInt (screenLayout.topBaseline);
    if (isMessageShownAlone())
    {
        const auto message = messages.current (now());
        juce::Graphics::ScopedSaveState saved (g);
        g.reduceClipRegion (area.withHeight (screenLayout.topBaseline - area.getY() + topFont.getDescent()).toNearestInt());
        const auto offset = StageText::marqueeOffset (messageOverflow, scaled (messageScrollSpeed), messages.elapsedMs (now()));
        g.drawSingleLineText (message, juce::roundToInt (area.getX() - offset), topBaseline);
    }
    else
    {
        const auto topLeft  = getTopLineText();
        const auto topRight = topRightText();
        const auto leftWidth   = juce::GlyphArrangement::getStringWidth (topFont, topLeft);
        const auto middleWidth = juce::GlyphArrangement::getStringWidth (topFont, content.mode);
        const auto rightWidth  = juce::GlyphArrangement::getStringWidth (topFont, topRight);
        const auto topGap = (area.getWidth() - leftWidth - middleWidth - rightWidth) / halves;
        g.drawSingleLineText (topLeft, juce::roundToInt (area.getX()), topBaseline);
        g.drawSingleLineText (content.mode, juce::roundToInt (area.getX() + leftWidth + topGap), topBaseline);
        g.drawSingleLineText (topRight, juce::roundToInt (area.getRight()), topBaseline, juce::Justification::right);
    }

    // Title row: style name left, tempo right, on one baseline.
    g.setFont (StageFonts::lcd (scaled (lcdTitleSize)));
    g.drawSingleLineText (content.styleName, juce::roundToInt (area.getX()), juce::roundToInt (screenLayout.titleBaseline));
    if (editingTempo)
    {
        g.setColour (StagePalette::lcdInk);
        g.fillRoundedRectangle (tempoEditor.getBounds().toFloat(), scaled (lcdCornerRadius));   // ink is also the text colour below
    }
    else if (tempoVisible())
    {
        const auto tempoArea = getTempoBounds();
        if (mouseOverTempo)
        {
            g.setColour (StagePalette::lcdInk.withAlpha (tempoHoverAlpha));
            g.fillRoundedRectangle (tempoArea, scaled (lcdCornerRadius));
            g.setColour (StagePalette::lcdInk);
        }
        g.setFont (StageFonts::lcd (scaled (lcdTempoSize)));
        g.drawSingleLineText (tempoText(), juce::roundToInt (tempoArea.getRight() - scaled (tempoPaddingSide)),
                              juce::roundToInt (screenLayout.titleBaseline), juce::Justification::right);
    }

    // Bottom row: variation, chord, then the bar at the right.
    const auto bottomFont = StageFonts::lcd (scaled (lcdBottomRowSize));
    g.setFont (bottomFont);
    const auto bottomBaseline = juce::roundToInt (screenLayout.bottomBaseline);
    g.drawSingleLineText (content.variation, juce::roundToInt (area.getX()), bottomBaseline);
    // With no variation shown the chord starts at the row's edge (owner: it was indented by the gap).
    const auto chordX = content.variation.isEmpty()
                          ? area.getX()
                          : area.getX() + juce::GlyphArrangement::getStringWidth (bottomFont, content.variation) + scaled (lcdBottomRowGap);
    g.drawSingleLineText (content.chord, juce::roundToInt (chordX), bottomBaseline);
    const auto barText = barPrefix + juce::String (content.bar).paddedLeft (zeroDigit.getLastCharacter(), barDigits);
    g.drawSingleLineText (barText, juce::roundToInt (area.getRight()), bottomBaseline, juce::Justification::right);

    paintLamps (g, screenLayout.lampRow);
}

void StageScreen::paintLamps (juce::Graphics& g, juce::Rectangle<float> lampRow) const
{
    using namespace StageMetrics;
    const auto lamps = BeatLampModel::lamps (content.beatsPerBar, content.elapsedBeats, content.playing, content.asleep);
    if (lamps.empty())
        return;

    const auto gap    = scaled (lampGap);
    const auto count  = static_cast<float> (lamps.size());
    const auto width  = (lampRow.getWidth() + gap) / count - gap;   // count lamps and count - 1 gaps fill the row
    const auto corner = scaled (lampCornerRadius);
    auto left = lampRow.getX();

    for (const auto lampState : lamps)
    {
        const juce::Rectangle<float> lamp { left, lampRow.getY(), width, lampRow.getHeight() };
        left += width + gap;

        if (lampState == LampState::off)
        {
            g.setColour (juce::Colours::white.withAlpha (lampEdgeAlpha));
            g.fillRoundedRectangle (lamp.translated (float {}, scaled (hairline)), corner);
            g.setColour (StagePalette::lampOff);
            g.fillRoundedRectangle (lamp, corner);
            g.setColour (juce::Colours::black.withAlpha (lampShadeAlpha));
            g.fillRect (lamp.reduced (corner, float {}).withHeight (scaled (hairline)));
            continue;
        }

        const auto downbeat = lampState == LampState::downbeat;
        juce::Path lampShape;
        lampShape.addRoundedRectangle (lamp, corner);
        juce::DropShadow (downbeat ? StagePalette::lampDownbeatGlow : StagePalette::lampBeatGlow,
                          juce::roundToInt (scaled (lampGlowRadius)), {}).drawForPath (g, lampShape);
        g.setGradientFill (juce::ColourGradient::vertical (downbeat ? StagePalette::lampDownbeatTop : StagePalette::lampBeatTop, lamp.getY(),
                                                           downbeat ? StagePalette::lampDownbeatBottom : StagePalette::lampBeatBottom, lamp.getBottom()));
        g.fillPath (lampShape);
        g.setColour (juce::Colours::white.withAlpha (downbeat ? downbeatShineAlpha : beatShineAlpha));
        g.fillRect (lamp.reduced (corner, float {}).withHeight (scaled (hairline)));
    }
}

//==============================================================================
void StageScreen::openTempoEditor()
{
    if (tempoModel.isLocked())
    {
        postMessage (lockedMessage);
        return;
    }

    editingTempo = true;
    tempoEditor.setText (tempoText(), juce::dontSendNotification);
    resized();
    tempoEditor.setVisible (true);
    if (tempoEditor.isShowing())
        tempoEditor.grabKeyboardFocus();
    tempoEditor.selectAll();
    repaint();
}

void StageScreen::closeTempoEditor (bool apply)
{
    if (! editingTempo)
        return;

    editingTempo = false;   // first: hiding the editor makes it lose focus, which calls back here
    const auto typed = tempoEditor.getText();
    tempoEditor.setVisible (false);
    if (apply)
        reportTempoChange (tempoModel.applyTyped (typed));
    repaint();
}

void StageScreen::mouseMove (const juce::MouseEvent& event)
{
    const auto over = getTempoBounds().contains (event.position);
    setMouseCursor (over ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
    if (over != mouseOverTempo)
    {
        mouseOverTempo = over;
        repaint();
    }
}

void StageScreen::mouseExit (const juce::MouseEvent&)
{
    if (mouseOverTempo)
    {
        mouseOverTempo = false;
        repaint();
    }
}

void StageScreen::mouseDown (const juce::MouseEvent& event)
{
    pressedOnTempo = false;
    if (! getTempoBounds().contains (event.position))
        return;

    if (event.mods.isPopupMenu())
    {
        if (tempoModel.isLocked())
        {
            postMessage (lockedMessage);
            return;
        }

        const auto changed = tempoModel.resetToStyle();
        postMessage (backToStylePrefix + tempoText());
        reportTempoChange (changed);
        return;
    }

    pressedOnTempo = true;
    draggingTempo  = false;
    dragStepsTaken = {};
}

void StageScreen::mouseDrag (const juce::MouseEvent& event)
{
    if (! pressedOnTempo)
        return;

    // Up = faster: one step per tempoDragStep px of vertical movement.
    const auto stepsSoFar = static_cast<int> (-event.getDistanceFromDragStartY() / scaled (StageMetrics::tempoDragStep));
    if (stepsSoFar == dragStepsTaken)
        return;

    if (! draggingTempo)
        beginTempoGesture();
    draggingTempo = true;
    const auto direction = stepsSoFar > dragStepsTaken ? StageMetrics::tempoStepUp : StageMetrics::tempoStepDown;
    while (dragStepsTaken != stepsSoFar)
    {
        dragStepsTaken += direction;
        if (! stepTempo (direction) && tempoModel.isLocked())
            break;
    }
    dragStepsTaken = stepsSoFar;
}

void StageScreen::mouseUp (const juce::MouseEvent&)
{
    const auto wasClick = pressedOnTempo && ! draggingTempo;
    if (draggingTempo)
        endTempoGesture();
    pressedOnTempo = false;
    draggingTempo  = false;
    if (wasClick)
        openTempoEditor();
}

void StageScreen::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (! getTempoBounds().contains (event.position) || juce::exactlyEqual (wheel.deltaY, float {}))
        return;

    const auto notches = wheel.deltaY > float {} ? StageMetrics::tempoStepUp : StageMetrics::tempoStepDown;
    reportTempoChange (tempoModel.wheel (notches, event.mods.isShiftDown()));
}
