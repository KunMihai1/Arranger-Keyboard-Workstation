#pragma once

#include <JuceHeader.h>

/*  The app's notices and questions as StageDialogs (page materials) instead of juce::AlertWindow, which stalled the
    UI 0.5 - 1 s per box in Debug (it lays its text out once per pixel of width) and looked like a different program.

    One dialog at a time, app-wide: a new one replaces the one showing (without running its callbacks). It covers the
    whole main scene, as the old modal boxes did, so nothing under it (a page, the panel) can change while it shows.
    An anchor in another window (e.g. the Add Device window) puts it in that window instead. */
namespace StageAlert
{
    /** A notice with one OK button; Enter, Esc or a click outside closes it. onOk runs after it closes. */
    void showMessage (const juce::String& title, const juce::String& message,
                      juce::Component* anchor = nullptr, std::function<void()> onOk = {});

    /** A question with Cancel and confirmText; onConfirm runs only when confirmed. */
    void showQuestion (const juce::String& title, const juce::String& message, const juce::String& confirmText,
                       std::function<void()> onConfirm, juce::Component* anchor = nullptr);

    /** A question with Cancel, alternativeText and confirmText (e.g. Change selected / Change All). */
    void showChoice (const juce::String& title, const juce::String& message, const juce::String& confirmText,
                     const juce::String& alternativeText, std::function<void()> onConfirm,
                     std::function<void()> onAlternative, juce::Component* anchor = nullptr);

    /** A question with an LCD text field; onConfirm gets the trimmed text. */
    void showInput (const juce::String& title, const juce::String& label, const juce::String& confirmText,
                    const juce::String& startText, std::function<void (const juce::String&)> onConfirm,
                    juce::Component* anchor = nullptr);

    /** The main scene the alerts cover (set by the main window; tests leave it unset and pass an anchor). */
    void setMainHost (juce::Component* mainScene);

    /** Set by the main window: what to do while a dialog covers the scene (the GL note layer would draw over it). */
    void setCoverHooks (std::function<void()> onCover, std::function<void()> onUncover);

    /** Closes and frees every dialog (before the main window goes away). */
    void closeAll();

    /** The dialog showing now, or nullptr (tests). */
    juce::Component* current();
}
