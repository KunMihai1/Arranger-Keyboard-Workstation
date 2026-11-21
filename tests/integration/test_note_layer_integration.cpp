#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "MidiHandler.h"
#include "KeyboardUI.h"
#include "NoteLayer.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>

// ==================================================================
// INTEGRATION (local only: needs a real display and a GPU)
// Puts the real KeyboardUI + NoteLayer on screen, plays notes through the public API and reads
// the pixels back, to check the render-thread drawing end to end: notes appear in their key's
// column, leave the screen after release, and survive the hide/show cycle that detaches and
// re-creates the GL context (every overlay in MainComponent does that).
// ==================================================================

namespace
{
    /** The test runner has no message loop running, so pump the Win32 queue by hand. */
    void pumpMessages (int milliseconds)
    {
        const auto end = juce::Time::getMillisecondCounter() + (juce::uint32) milliseconds;
        while (juce::Time::getMillisecondCounter() < end)
        {
            MSG msg;
            while (PeekMessage (&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage (&msg);
                DispatchMessage (&msg);
            }
            juce::Thread::sleep (2);
        }
    }

    /** Brightest pixel (max of r, g, b) on screen inside a logical-pixel rectangle. */
    int brightestOnScreen (juce::Rectangle<int> logical)
    {
        const double scale = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;
        const auto r = (logical.toDouble() * scale).toNearestInt();

        HDC screen = GetDC (nullptr);
        HDC mem = CreateCompatibleDC (screen);
        HBITMAP bmp = CreateCompatibleBitmap (screen, r.getWidth(), r.getHeight());
        auto old = SelectObject (mem, bmp);
        BitBlt (mem, 0, 0, r.getWidth(), r.getHeight(), screen, r.getX(), r.getY(), SRCCOPY);
        SelectObject (mem, old);

        BITMAPINFO bi {};
        bi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = r.getWidth();
        bi.bmiHeader.biHeight = -r.getHeight();
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        std::vector<juce::uint8> px ((size_t) r.getWidth() * (size_t) r.getHeight() * 4);
        GetDIBits (mem, bmp, 0, (UINT) r.getHeight(), px.data(), &bi, DIB_RGB_COLORS);
        DeleteObject (bmp);
        DeleteDC (mem);
        ReleaseDC (nullptr, screen);

        int best = 0;
        for (size_t i = 0; i < px.size(); i += 4)
            best = std::max ({ best, (int) px[i], (int) px[i + 1], (int) px[i + 2] });
        return best;
    }

    struct BlackHost : public juce::Component
    {
        void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
    };
}

class NoteLayerIntegrationTest : public juce::UnitTest
{
public:
    NoteLayerIntegrationTest() : juce::UnitTest ("NoteLayer GL rendering (needs a display)", "Integration") {}

    void runTest() override
    {
        MidiDevice device;
        MidiHandler handler (device);
        KeyboardUI keyboard (handler);
        NoteLayer layer (keyboard);

        BlackHost host;
        host.setOpaque (true);
        host.addAndMakeVisible (keyboard);
        host.addAndMakeVisible (layer);
        host.setBounds (40, 40, 1040, 560);
        layer.setBounds (0, 0, 1040, 400);
        keyboard.setBounds (0, 400, 1040, 160);
        host.addToDesktop (0);
        host.setAlwaysOnTop (true);
        host.setVisible (true);
        host.toFront (false);
        pumpMessages (800); // window + GL context come up

        // A white key's column inside the note layer, in screen coordinates. Same rule as
        // KeyboardUI::resized(): 52 white keys across the width, A0 (MIDI 21) first.
        auto column = [&] (int midiNote)
        {
            static const int whiteIndexInOctave[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 }; // from C
            const int octave = midiNote / 12 - 1, pc = midiNote % 12;
            jassert (whiteIndexInOctave[pc] >= 0);
            const int whiteIndex = 2 + (octave - 1) * 7 + whiteIndexInOctave[pc]; // A0, B0, then C1 = 2
            const float whiteWidth = (float) keyboard.getWidth() / 52.0f;
            const int x = (int) (whiteIndex * whiteWidth);
            return layer.getScreenBounds().withX (layer.getScreenX() + x + 3).withWidth ((int) whiteWidth - 6);
        };
        const int lit = 150, dark = 30;

        beginTest ("held notes are drawn in their own key's column, nowhere else");
        {
            layer.noteOnReceived (60);
            layer.noteOnReceived (64);
            pumpMessages (500);
            expectGreaterThan (brightestOnScreen (column (60)), lit);
            expectGreaterThan (brightestOnScreen (column (64)), lit);
            expectLessThan (brightestOnScreen (column (96)), dark);
        }

        beginTest ("released notes rise off the screen and the layer clears");
        {
            layer.noteOffReceived (60);
            layer.noteOffReceived (64);
            pumpMessages (2500);
            expectLessThan (brightestOnScreen (layer.getScreenBounds()), dark);
        }

        beginTest ("drawing survives the hide/show cycle that re-creates the GL context");
        {
            layer.setVisible (false);
            pumpMessages (300);
            layer.setVisible (true);
            layer.resetState();
            pumpMessages (500);
            layer.noteOnReceived (72);
            pumpMessages (500);
            expectGreaterThan (brightestOnScreen (column (72)), lit);
        }

        beginTest ("resetStateActiveNotes releases held notes; resetState clears at once");
        {
            layer.resetStateActiveNotes();
            pumpMessages (2500);
            expectLessThan (brightestOnScreen (layer.getScreenBounds()), dark);

            layer.noteOnReceived (48);
            pumpMessages (400);
            expectGreaterThan (brightestOnScreen (column (48)), lit);
            layer.resetState();
            pumpMessages (200);
            expectLessThan (brightestOnScreen (layer.getScreenBounds()), dark);
        }

        host.removeFromDesktop();
        host.removeAllChildren();
        pumpMessages (100);
    }
};

static NoteLayerIntegrationTest noteLayerIntegrationTest;

#endif
