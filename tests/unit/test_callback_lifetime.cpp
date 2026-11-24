#include <JuceHeader.h>
#include "Arranger/ArrangerEngine.h"
#include "TrackPlayer.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
#endif

/*  Beat-bar updates are posted to the message thread. If the engine or player is destroyed before the
    posted message runs, the message must do nothing (it used to call into the freed object: the crash
    when deleting the open style). */

namespace CallbackLifetimeTestValues
{
    const juce::String testName     = "CallbackLifetime";
    const juce::String testCategory = "Unit";

    const juce::String engineStop = "ArrangerEngine: a stop posted before destruction does not call back afterwards";
    const juce::String playerStop = "TrackPlayer: a stop posted before destruction does not call back afterwards";

    constexpr int pumpMilliseconds = 200;
    constexpr int pollIntervalMs   = 2;
    constexpr juce::uint32 noMessageFilter = 0;
}

class CallbackLifetimeTest : public juce::UnitTest
{
public:
    CallbackLifetimeTest() : juce::UnitTest (CallbackLifetimeTestValues::testName, CallbackLifetimeTestValues::testCategory) {}

    /** The test runner has no message loop running, so pump the Win32 queue by hand. */
    static void pumpMessagesFor (int milliseconds)
    {
        using namespace CallbackLifetimeTestValues;
        const auto end = juce::Time::getMillisecondCounter() + static_cast<juce::uint32> (milliseconds);
        while (juce::Time::getMillisecondCounter() < end)
        {
           #if JUCE_WINDOWS
            MSG message;
            while (PeekMessage (&message, nullptr, noMessageFilter, noMessageFilter, PM_REMOVE))
            {
                TranslateMessage (&message);
                DispatchMessage (&message);
            }
           #endif
            juce::Thread::sleep (pollIntervalMs);
        }
    }

    void runTest() override
    {
        using namespace CallbackLifetimeTestValues;

        beginTest (engineStop);
        {
            auto ran = std::make_shared<bool> (false);
            {
                ArrangerEngine engine (std::weak_ptr<juce::MidiOutput> {});
                engine.onElapsedBeats = [ran] (double) { *ran = true; };
                engine.stop();
            }
            pumpMessagesFor (pumpMilliseconds);
            expect (! *ran);
        }

        beginTest (playerStop);
        {
            auto ran = std::make_shared<bool> (false);
            {
                MultipleTrackPlayer player (std::weak_ptr<juce::MidiOutput> {});
                player.onElapsedUpdate = [ran] (double) { *ran = true; };
                player.stop();
            }
            pumpMessagesFor (pumpMilliseconds);
            expect (! *ran);
        }
    }
};
static CallbackLifetimeTest callbackLifetimeTest;
