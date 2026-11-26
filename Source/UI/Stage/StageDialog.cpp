#include "StageDialog.h"
#include "StageFonts.h"
#include "StagePageFrame.h"
#include "Common/AppColours.h"

namespace StageDialogText
{
    const juce::String cancel = "Cancel";
}

namespace
{
    // Board px at scale 1: the panel and its rows, top to bottom.
    constexpr int   panelWidth      = 400;
    constexpr int   panelPadding    = 18;
    constexpr int   headingHeight   = 12;
    constexpr int   headingGap      = 14;
    constexpr int   labelToInputGap = 6;
    constexpr int   inputHeight     = 24;
    constexpr int   buttonsGap      = 20;
    constexpr int   buttonGap       = 6;
    constexpr float panelCorner     = 4.0f;
    constexpr float messageTextSize = 13.0f;
    constexpr float messageSpacing  = 0.02f;
    constexpr float backdropAlpha   = 0.55f;
    constexpr float shadowAlpha     = 0.6f;
    constexpr int   shadowRadius    = 18;
    constexpr int   shadowDrop      = 6;
    constexpr int   halves          = 2;
    constexpr bool  regularWeight   = false;
    constexpr bool  semiBoldWeight  = true;

    int scaledLength (int boardLength, float scale) { return juce::roundToInt (static_cast<float> (boardLength) * scale); }
}

StageDialogRects stageDialogLayout (juce::Rectangle<int> host, float scale, bool hasInput, float messageHeight)
{
    const auto padding = scaledLength (panelPadding, scale);
    const auto message = juce::roundToInt (std::ceil (messageHeight));
    auto height = padding + scaledLength (headingHeight, scale) + scaledLength (headingGap, scale) + message
                + scaledLength (buttonsGap, scale) + scaledLength (StageMetrics::segHeight, scale) + padding;
    if (hasInput)
        height += scaledLength (labelToInputGap, scale) + scaledLength (inputHeight, scale);

    StageDialogRects rects;
    rects.panel = host.withSizeKeepingCentre (scaledLength (panelWidth, scale), height);
    auto area = rects.panel.reduced (padding);
    rects.heading = area.removeFromTop (scaledLength (headingHeight, scale));
    area.removeFromTop (scaledLength (headingGap, scale));
    rects.message = area.removeFromTop (message);
    if (hasInput)
    {
        area.removeFromTop (scaledLength (labelToInputGap, scale));
        rects.input = area.removeFromTop (scaledLength (inputHeight, scale));
    }
    area.removeFromTop (scaledLength (buttonsGap, scale));
    rects.buttonRow = area.removeFromTop (scaledLength (StageMetrics::segHeight, scale));
    return rects;
}

StageDialog::StageDialog (StageDialogOptions dialogOptions, OnConfirm confirmCallback)
    : options (std::move (dialogOptions)), onConfirm (std::move (confirmCallback)),
      input ({}),
      cancelButton (StageDialogText::cancel, SegButton::Style::seg),
      confirmButton (options.confirmText, SegButton::Style::lit)
{
    setWantsKeyboardFocus (true);
    setMouseClickGrabsKeyboardFocus (false);   // a click on the panel must not take the typing away from the input
    input.setText (options.inputText, false);
    input.onReturnKey = [this] { confirm(); };
    input.onEscapeKey = [this] { cancel(); };
    addChildComponent (input);
    input.setVisible (options.hasInput);
    cancelButton.onClick  = [this] { cancel(); };
    confirmButton.onClick = [this] { confirm(); };
    addAndMakeVisible (cancelButton);
    addAndMakeVisible (confirmButton);
}

StageDialog::~StageDialog() = default;

void StageDialog::showOver (juce::Component& anchor, float newScale)
{
    scale = newScale;
    input.setScale (scale);
    cancelButton.setScale (scale);
    confirmButton.setScale (scale);

    auto* host = dynamic_cast<StagePageFrame*> (&anchor) != nullptr ? &anchor
                                                                     : anchor.findParentComponentOfClass<StagePageFrame>();
    if (host == nullptr)
        host = anchor.getTopLevelComponent();

    host->addAndMakeVisible (this);
    setBounds (host->getLocalBounds());
    toFront (true);
    if (options.hasInput)
    {
        input.grabKeyboardFocus();
        input.selectAll();
    }
    else
    {
        grabKeyboardFocus();
    }
}

void StageDialog::confirm()
{
    if (! isVisible())
        return;
    const auto text = getText();
    setVisible (false);
    if (onConfirm)
        onConfirm (text);   // may delete what opened the dialog: touch nothing after this
}

void StageDialog::cancel()
{
    setVisible (false);
}

bool StageDialog::hasInput() const { return options.hasInput; }

juce::String StageDialog::getText() const
{
    return options.hasInput ? input.getText().trim() : juce::String();
}

juce::Rectangle<int> StageDialog::getPanelBounds() const
{
    return stageDialogLayout (getLocalBounds(), scale, options.hasInput, messageHeight()).panel;
}

SegButton& StageDialog::getConfirmButton() { return confirmButton; }
SegButton& StageDialog::getCancelButton()  { return cancelButton; }

juce::TextLayout StageDialog::messageLayout (float width) const
{
    juce::AttributedString text;
    text.append (options.message, StageFonts::silkscreen (messageTextSize * scale, regularWeight, messageSpacing),
                 StagePalette::silkscreenText);
    text.setWordWrap (juce::AttributedString::byWord);
    juce::TextLayout layout;
    layout.createLayout (text, width);   // one layout, not AlertWindow's balanced search
    return layout;
}

float StageDialog::messageHeight() const
{
    if (options.hasInput)   // the input's label: one silkscreen line
        return StageFonts::silkscreen (StageMetrics::silkLabelSize * scale, semiBoldWeight, StageMetrics::silkDimSpacing).getHeight();
    const auto width = static_cast<float> (scaledLength (panelWidth - panelPadding * halves, scale));
    return messageLayout (width).getHeight();
}

void StageDialog::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (backdropAlpha));

    const auto rects = stageDialogLayout (getLocalBounds(), scale, options.hasInput, messageHeight());
    const auto panel = rects.panel.toFloat();
    const auto corner = panelCorner * scale;

    juce::Path outline;
    outline.addRoundedRectangle (panel, corner);
    juce::DropShadow (juce::Colours::black.withAlpha (shadowAlpha), scaledLength (shadowRadius, scale), { int {}, scaledLength (shadowDrop, scale) })
        .drawForPath (g, outline);
    g.setGradientFill (juce::ColourGradient::vertical (StagePalette::pageTop, panel.getY(), StagePalette::pageBottom, panel.getBottom()));
    g.fillPath (outline);
    g.setColour (StagePalette::ruleLine);
    g.drawRoundedRectangle (panel, corner, static_cast<float> (StageMetrics::ruleWidth) * scale);

    StageLcd::paintHeading (g, rects.heading.toFloat(), scale, options.title, {});

    if (options.hasInput)
    {
        g.setColour (StagePalette::silkscreenDimText);
        g.setFont (StageFonts::silkscreen (StageMetrics::silkLabelSize * scale, semiBoldWeight, StageMetrics::silkDimSpacing));
        g.drawText (options.message.toUpperCase(), rects.message, juce::Justification::centredLeft, true);
    }
    else
    {
        messageLayout (static_cast<float> (rects.message.getWidth())).draw (g, rects.message.toFloat());
    }
}

void StageDialog::resized()
{
    const auto rects = stageDialogLayout (getLocalBounds(), scale, options.hasInput, messageHeight());
    input.setBounds (rects.input);

    auto right = static_cast<float> (rects.buttonRow.getRight());
    const auto top = static_cast<float> (rects.buttonRow.getY());
    const auto height = static_cast<float> (rects.buttonRow.getHeight());
    for (auto* button : { &confirmButton, &cancelButton })   // right to left, adjacent with a gap
    {
        const auto width = button->getExactWidth();
        button->setBounds (juce::Rectangle<float> (right - width, top, width, height).toNearestInt());
        right -= width + static_cast<float> (buttonGap) * scale;
    }
}

bool StageDialog::isInsideDialog (const juce::Component* component)
{
    for (auto* ancestor = component; ancestor != nullptr; ancestor = ancestor->getParentComponent())
        if (dynamic_cast<const StageDialog*> (ancestor) != nullptr)
            return true;
    return false;
}

void StageDialog::parentSizeChanged()
{
    if (auto* host = getParentComponent())
        setBounds (host->getLocalBounds());   // the page frame does not lay out its overlays
}

bool StageDialog::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::returnKey)
    {
        confirm();
        return true;
    }
    if (key == juce::KeyPress::escapeKey)
    {
        cancel();
        return true;
    }
    return true;   // modal over the page: no key reaches what is behind it
}

void StageDialog::mouseDown (const juce::MouseEvent& event)
{
    if (! getPanelBounds().contains (event.getPosition()))
        cancel();   // a click on the dimmed page
}
