#include "StageAlert.h"
#include "StageDialog.h"
#include "StageScale.h"

namespace
{
    namespace AlertText
    {
        const juce::String ok = "OK";
    }

    struct AlertState
    {
        std::unique_ptr<StageDialog> showing;
        std::vector<std::unique_ptr<StageDialog>> retired;   // replaced while possibly running its own callback
        std::function<void()> onCover, onUncover;
        juce::Component::SafePointer<juce::Component> mainHost;
        bool covering = false;
    };

    AlertState& state()
    {
        static AlertState alertState;
        return alertState;
    }

    void uncover()
    {
        auto& alerts = state();
        if (alerts.covering && alerts.onUncover)
            alerts.onUncover();
        alerts.covering = false;
    }

    /** Frees replaced dialogs later: one may be the dialog whose button callback opened the new one. */
    void releaseRetiredLater()
    {
        juce::MessageManager::callAsync ([] { state().retired.clear(); });
    }

    juce::Component* hostFor (juce::Component* anchor)
    {
        auto* mainHost = state().mainHost.getComponent();
        const bool anchorInAnotherWindow = anchor != nullptr
                                        && (mainHost == nullptr || anchor->getTopLevelComponent() != mainHost->getTopLevelComponent());
        if (anchorInAnotherWindow)
            return anchor;
        if (mainHost != nullptr)
            return mainHost;   // the whole scene, not the page the anchor is on: a page can't close under it
        if (anchor != nullptr)
            return anchor;
        if (auto* window = juce::TopLevelWindow::getActiveTopLevelWindow())
            return window;
        return juce::TopLevelWindow::getNumTopLevelWindows() > int {} ? juce::TopLevelWindow::getTopLevelWindow (int {}) : nullptr;
    }

    void show (StageDialogOptions options, StageDialog::OnConfirm onConfirm, juce::Component* anchor,
               std::function<void()> onAlternative = {})
    {
        if (! juce::MessageManager::existsAndIsCurrentThread())
        {
            // A worker thread's notice (the old AlertWindow calls hopped threads the same way): show it from the
            // message thread, over the main window (an anchor could be gone by then).
            juce::MessageManager::callAsync ([options, onConfirm, onAlternative]
            {
                show (options, onConfirm, nullptr, onAlternative);
            });
            return;
        }

        auto* host = hostFor (anchor);
        if (host == nullptr)
            return;   // no window (shutting down): nothing to show it over

        auto& alerts = state();
        if (alerts.showing != nullptr)
        {
            alerts.showing->setVisible (false);
            alerts.retired.push_back (std::move (alerts.showing));
            releaseRetiredLater();
        }

        alerts.showing = std::make_unique<StageDialog> (std::move (options), std::move (onConfirm));
        alerts.showing->onClosed = [] { uncover(); };
        alerts.showing->onAlternative = std::move (onAlternative);

        auto* window = dynamic_cast<juce::ResizableWindow*> (host->getTopLevelComponent());
        auto* scene = window != nullptr && window->getContentComponent() != nullptr ? window->getContentComponent()
                                                                                    : host->getTopLevelComponent();
        const auto scale = StageScale::regionsFor (scene->getLocalBounds()).scale;   // the app's own scale
        alerts.showing->showOver (*host, scale);

        if (! alerts.covering && alerts.onCover)
            alerts.onCover();   // after showOver: the hooks see it as the alert showing
        alerts.covering = true;
    }
}

void StageAlert::showMessage (const juce::String& title, const juce::String& message, juce::Component* anchor,
                              std::function<void()> onOk)
{
    StageDialogOptions options { title, message, AlertText::ok };
    options.hasCancel = false;
    show (std::move (options), [onOk = std::move (onOk)] (const juce::String&) { if (onOk) onOk(); }, anchor);
}

void StageAlert::showQuestion (const juce::String& title, const juce::String& message, const juce::String& confirmText,
                               std::function<void()> onConfirm, juce::Component* anchor)
{
    show ({ title, message, confirmText }, [onConfirm = std::move (onConfirm)] (const juce::String&) { if (onConfirm) onConfirm(); },
          anchor);
}

void StageAlert::showChoice (const juce::String& title, const juce::String& message, const juce::String& confirmText,
                             const juce::String& alternativeText, std::function<void()> onConfirm,
                             std::function<void()> onAlternative, juce::Component* anchor)
{
    StageDialogOptions options { title, message, confirmText };
    options.alternativeText = alternativeText;
    show (std::move (options), [onConfirm = std::move (onConfirm)] (const juce::String&) { if (onConfirm) onConfirm(); },
          anchor, std::move (onAlternative));
}

void StageAlert::showInput (const juce::String& title, const juce::String& label, const juce::String& confirmText,
                            const juce::String& startText, std::function<void (const juce::String&)> onConfirm,
                            juce::Component* anchor)
{
    constexpr bool withInput = true;
    show ({ title, label, confirmText, withInput, startText }, std::move (onConfirm), anchor);
}

void StageAlert::setMainHost (juce::Component* mainScene)
{
    state().mainHost = mainScene;
}

void StageAlert::setCoverHooks (std::function<void()> onCover, std::function<void()> onUncover)
{
    state().onCover = std::move (onCover);
    state().onUncover = std::move (onUncover);
}

void StageAlert::closeAll()
{
    auto& alerts = state();
    uncover();
    alerts.showing.reset();
    alerts.retired.clear();
    alerts.onCover = nullptr;
    alerts.onUncover = nullptr;
    alerts.mainHost = nullptr;
}

juce::Component* StageAlert::current()
{
    auto& alerts = state();
    if (alerts.showing == nullptr || ! alerts.showing->isVisible())
        return nullptr;
    // Not stranded on a hidden part of the scene (the top level itself may be off screen, as in tests).
    for (auto* ancestor = alerts.showing->getParentComponent(); ancestor != nullptr && ancestor->getParentComponent() != nullptr;
         ancestor = ancestor->getParentComponent())
        if (! ancestor->isVisible())
            return nullptr;
    return alerts.showing.get();
}
