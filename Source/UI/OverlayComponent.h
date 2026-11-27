#pragma once
#include <JuceHeader.h>
#include <array>
#include <functional>
#include "Stage/StagePageWidgets.h"

inline constexpr size_t escMenuButtonCount = 3;   ///< Settings, Effects, Exit

/** Board-px layout of the Esc menu over a host of the given bounds, at the app's uniform scale. */
struct EscMenuRects
{
    juce::Rectangle<int> panel;
    juce::Rectangle<int> heading;
    std::array<juce::Rectangle<int>, escMenuButtonCount> buttons;   ///< top to bottom
};

EscMenuRects escMenuLayout (juce::Rectangle<int> host, float scale);

/*  The Esc menu (spec part 2 P.12): a small panel in the page materials over the dimmed app — silkscreen heading,
    then Settings, Effects (they open their pages) and Exit as seg buttons. Esc or a click outside the panel closes
    it (owner, 2026-11-27: "when pressing esc the old screen opens"). */
class OverlayComponent : public juce::Component
{
public:
    std::function<void()> onSettingsClick;
    std::function<void()> onEffectsClick;
    std::function<bool()> windowExists;
    std::function<void()> closeWindow;
    std::function<void()> bringSeparateWindowFront;
    std::function<void()> setWindowFlag;

    OverlayComponent();
    ~OverlayComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void showOverlay();
    void hideOverlay();

    /** The app's uniform scale (StageRegions::scale), so the menu looks the same on any window. */
    void setScale (float newScale);

    SegButton& getExitButton();
    SegButton& getSettingsButton();
    SegButton& getEffectsButton();

    void mouseDown(const juce::MouseEvent& ev) override;
    /** A click at `position`: outside the panel it closes the menu, as Esc does. */
    void clickAt (juce::Point<int> position);

    bool keyPressed(const juce::KeyPress& key) override;

    // main component should assign this so overlay can request to be closed
    std::function<void()> onRequestClose;

private:
    void requestClose();

    float scale { StageMetrics::referenceScale };
    SegButton settingsButton;
    SegButton effectsButton;
    SegButton exitButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlayComponent)
};
