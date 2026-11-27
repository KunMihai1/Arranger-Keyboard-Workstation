/*
  ==============================================================================

    StageDialog.h

    An in-app dialog drawn over a page: a dimmed backdrop, a dark panel with a
    silkscreen heading, a message or an LCD input, and seg buttons (Cancel,
    confirm). Enter confirms, Esc or a click on the backdrop cancels.

    It replaces juce::AlertWindow on the Stage pages: an AlertWindow re-lays its
    title and message once per pixel of width on construction and on every
    addButton / addTextEditor, which stalled the UI for 0.6 - 1 s per dialog
    (measured 2026-11-25, see the ui-polish spec).

    The owner keeps the dialog (one at a time) and replaces it with the next one;
    closing only hides it, so a callback may delete what opened the dialog.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "StagePageWidgets.h"

struct StageDialogOptions
{
    juce::String title;          ///< the heading, e.g. "Rename folder"
    juce::String message;        ///< the question, or the input's label
    juce::String confirmText;    ///< the confirm button, e.g. "OK" / "Remove"
    bool         hasInput = false;
    juce::String inputText;      ///< the input's starting text
    bool         hasCancel = true;   ///< false: a notice with its one button (OK)
    juce::String alternativeText;    ///< a second action between Cancel and confirm (e.g. "Change selected")
};

/** Board-px layout of a dialog over a host of the given bounds. */
struct StageDialogRects
{
    juce::Rectangle<int> panel;
    juce::Rectangle<int> heading;
    juce::Rectangle<int> message;
    juce::Rectangle<int> input;     ///< empty without an input
    juce::Rectangle<int> buttonRow; ///< the buttons sit at its right end
};

/** messageHeight: the message's laid-out height at this scale (one line for an input's label). */
StageDialogRects stageDialogLayout (juce::Rectangle<int> host, float scale, bool hasInput, float messageHeight);

class StageDialog : public juce::Component
{
public:
    using OnConfirm = std::function<void (const juce::String& text)>;

    StageDialog (StageDialogOptions options, OnConfirm onConfirm);
    ~StageDialog() override;

    /** Covers the page that holds `anchor` (its StagePageFrame, else its top-level component) and takes the keys. */
    void showOver (juce::Component& anchor, float scale);

    void confirm();   ///< hides, then reports the input's trimmed text (empty without an input)
    void cancel();    ///< hides; nothing is reported (a notice without Cancel confirms instead)

    /** Called whenever the dialog closes (confirm, the alternative or cancel), before the action's callback. */
    std::function<void()> onClosed;
    /** The alternative button's action (options.alternativeText); the dialog has closed when it runs. */
    std::function<void()> onAlternative;

    bool hasInput() const;
    juce::String getText() const;
    juce::Rectangle<int> getPanelBounds() const;
    SegButton& getConfirmButton();
    SegButton& getCancelButton();

    void paint (juce::Graphics& g) override;
    void resized() override;
    /** The component is a dialog or inside one: the app must not pull keyboard focus away on its clicks. */
    static bool isInsideDialog (const juce::Component* component);

    bool keyPressed (const juce::KeyPress& key) override;
    void parentSizeChanged() override;
    void mouseDown (const juce::MouseEvent& event) override;

private:
    float messageHeight() const;
    juce::TextLayout messageLayout (float width) const;

    StageDialogOptions options;
    OnConfirm onConfirm;
    float scale { StageMetrics::referenceScale };

    StageLcdInput input;
    SegButton cancelButton;
    SegButton alternativeButton;
    SegButton confirmButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StageDialog)
};
