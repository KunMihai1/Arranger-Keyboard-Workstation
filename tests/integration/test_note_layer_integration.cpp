#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "MidiHandler.h"
#include "KeyboardUI.h"
#include "NoteLayer.h"
#include "ParticleRecipe.h"

namespace NoteLayerIntegrationTestValues
{
    const juce::String testName     = "NoteLayer GL rendering (needs a display)";
    const juce::String testCategory = "Integration-display";

    const juce::String heldNotesAreDrawnInTheirOwn = "held notes are drawn in their own key's column, nowhere else";
    const juce::String releasedNotesRiseOffTheScreenAnd = "released notes rise off the screen and the layer clears";
    const juce::String drawingSurvivesTheHideShowCycleThat = "drawing survives the hide/show cycle that re-creates the GL context";
    const juce::String resetStateActiveNotesReleasesHeldNotesResetStateClearsAt = "resetStateActiveNotes releases held notes; resetState clears at once";
    const juce::String heldHiddenStaysHidden = "held hidden (a page over the note area), a note does not show the layer; released, it does again";
    const juce::String sparksAreVisible = "sparks light up the area above the key (owner: they could not be seen)";
    const juce::String wispsAreVisible  = "light wisps light up the area above the key";

    constexpr juce::uint32 noMessageFilter = 0;
    constexpr int pollIntervalMs = 2;
    constexpr int bitmapOrigin = 0;
    constexpr juce::uint16 singlePlane = 1;
    constexpr juce::uint16 bitsPerPixel = 32;
    constexpr size_t bytesPerPixel = 4;
    constexpr juce::uint32 firstScanLine = 0;
    constexpr size_t blueByte = 0;
    constexpr size_t greenByte = 1;
    constexpr size_t redByte = 2;
    constexpr int hostX = 40;
    constexpr int hostY = 40;
    constexpr int hostWidth = 1040;
    constexpr int hostHeight = 560;
    constexpr int layerTop = 0;
    constexpr int layerHeight = 400;
    constexpr int keyboardHeight = 160;
    constexpr int noDesktopFlags = 0;
    constexpr int startUpMs = 800;
    constexpr int notesSettleMs = 500;
    constexpr int riseOffMs = 2500;
    constexpr int hideMs = 300;
    constexpr int noteAppearsMs = 400;
    constexpr int clearMs = 200;
    constexpr int closeMs = 100;
    constexpr int semitonesPerOctave = 12;
    constexpr int midiOctaveOffset = 1;   // MIDI note 0 is in octave -1
    constexpr int notWhite = -1;
    constexpr int whiteKeysBeforeC1 = 2;   // A0 and B0
    constexpr int firstFullOctave = 1;
    constexpr int whiteKeysPerOctave = 7;
    constexpr float totalWhiteKeys = 52.0f;
    constexpr int columnInset = 3;
    constexpr int columnTrim = 6;
    constexpr int litThreshold = 150;
    constexpr int darkThreshold = 30;
    constexpr int middleC = 60;
    constexpr int noteE4 = 64;
    constexpr int untouchedC7 = 96;
    constexpr int noteC5 = 72;
    constexpr int noteC3 = 48;
    constexpr int tapMs = 120;
    constexpr int particlesRiseMs = 450;
    constexpr int particlesGoneMs = 2600;
    constexpr int keysAround = 4;            // white keys either side of the played key
    constexpr int areaAboveKey = 300;        // logical px of the note area above the key
    constexpr int glowThreshold = 60;
    // Physical px lit by the particles beyond what the note itself lights. Sparks are a handful of small points, so they
    // are summed over several taps (worst run of ten: 782 px over three taps); wisps are long trails (1240+ px a tap).
    constexpr int sparkTaps            = 3;
    constexpr int minSparkPixelsPerTap = 150;
    constexpr int wispTaps             = 1;
    constexpr int minWispPixelsPerTap  = 400;

    // NoteLayer's default particle colour picks its alpha at random (0.6-1.0) once per layer: the tests use the dimmest.
    namespace DimmestDefaultParticle
    {
        constexpr float hue        = 0.12f;
        constexpr float saturation = 0.95f;
        constexpr float brightness = 0.9f;
        constexpr float alpha      = 0.6f;
    }
}

using namespace NoteLayerIntegrationTestValues;

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
            while (PeekMessage (&msg, nullptr, noMessageFilter, noMessageFilter, PM_REMOVE))
            {
                TranslateMessage (&msg);
                DispatchMessage (&msg);
            }
            juce::Thread::sleep (pollIntervalMs);
        }
    }

    /** Brightest pixel (max of r, g, b) on screen inside a logical-pixel rectangle. */
    int brightestOnScreen (juce::Rectangle<int> logical)
    {
        const double scale = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;
        const auto area = (logical.toDouble() * scale).toNearestInt();

        HDC screen = GetDC (nullptr);
        HDC mem = CreateCompatibleDC (screen);
        HBITMAP bmp = CreateCompatibleBitmap (screen, area.getWidth(), area.getHeight());
        auto old = SelectObject (mem, bmp);
        BitBlt (mem, bitmapOrigin, bitmapOrigin, area.getWidth(), area.getHeight(), screen, area.getX(), area.getY(), SRCCOPY);
        SelectObject (mem, old);

        BITMAPINFO bi {};
        bi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = area.getWidth();
        bi.bmiHeader.biHeight = -area.getHeight();
        bi.bmiHeader.biPlanes = singlePlane;
        bi.bmiHeader.biBitCount = bitsPerPixel;
        bi.bmiHeader.biCompression = BI_RGB;
        std::vector<juce::uint8> px ((size_t) area.getWidth() * (size_t) area.getHeight() * bytesPerPixel);
        GetDIBits (mem, bmp, firstScanLine, (UINT) area.getHeight(), px.data(), &bi, DIB_RGB_COLORS);
        DeleteObject (bmp);
        DeleteDC (mem);
        ReleaseDC (nullptr, screen);

        int best {};
        for (size_t pixel {}; pixel < px.size(); pixel += bytesPerPixel)
            best = std::max ({ best, (int) px[pixel + blueByte], (int) px[pixel + greenByte], (int) px[pixel + redByte] });
        return best;
    }

    /** Pixels on screen inside a logical-pixel rectangle brighter than `threshold` (max of r, g, b). */
    int litPixelsOnScreen (juce::Rectangle<int> logical, int threshold)
    {
        const double scale = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;
        const auto area = (logical.toDouble() * scale).toNearestInt();
        juce::Image shot (juce::Image::RGB, area.getWidth(), area.getHeight(), true);
        HDC screen = GetDC (nullptr);
        HDC mem = CreateCompatibleDC (screen);
        HBITMAP bmp = CreateCompatibleBitmap (screen, area.getWidth(), area.getHeight());
        auto old = SelectObject (mem, bmp);
        BitBlt (mem, bitmapOrigin, bitmapOrigin, area.getWidth(), area.getHeight(), screen, area.getX(), area.getY(), SRCCOPY);
        SelectObject (mem, old);
        BITMAPINFO bi {};
        bi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = area.getWidth();
        bi.bmiHeader.biHeight = -area.getHeight();
        bi.bmiHeader.biPlanes = singlePlane;
        bi.bmiHeader.biBitCount = bitsPerPixel;
        bi.bmiHeader.biCompression = BI_RGB;
        std::vector<juce::uint8> px ((size_t) area.getWidth() * (size_t) area.getHeight() * bytesPerPixel);
        GetDIBits (mem, bmp, firstScanLine, (UINT) area.getHeight(), px.data(), &bi, DIB_RGB_COLORS);
        DeleteObject (bmp);
        DeleteDC (mem);
        ReleaseDC (nullptr, screen);

        int lit {};
        for (size_t pixel {}; pixel < px.size(); pixel += bytesPerPixel)
            if (std::max ({ (int) px[pixel + blueByte], (int) px[pixel + greenByte], (int) px[pixel + redByte] }) > threshold)
                ++lit;
        return lit;
    }

    struct BlackHost : public juce::Component
    {
        void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
    };
}

class NoteLayerIntegrationTest : public juce::UnitTest
{
public:
    NoteLayerIntegrationTest() : juce::UnitTest (NoteLayerIntegrationTestValues::testName, NoteLayerIntegrationTestValues::testCategory) {}

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
        host.setBounds (hostX, hostY, hostWidth, hostHeight);
        layer.setBounds (layerTop, layerTop, hostWidth, layerHeight);
        keyboard.setBounds (layerTop, layerHeight, hostWidth, keyboardHeight);
        host.addToDesktop (noDesktopFlags);
        host.setAlwaysOnTop (true);
        host.setVisible (true);
        host.toFront (false);
        pumpMessages (startUpMs); // window + GL context come up

        // A white key's column inside the note layer, in screen coordinates. Same rule as
        // KeyboardUI::resized(): 52 white keys across the width, A0 (MIDI 21) first.
        auto column = [&] (int midiNote)
        {
            static const int whiteIndexInOctave[] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 }; // from C; -1 = black key
            const int octave = midiNote / semitonesPerOctave - midiOctaveOffset, pc = midiNote % semitonesPerOctave;
            jassert (whiteIndexInOctave[pc] > notWhite);
            const int whiteIndex = whiteKeysBeforeC1 + (octave - firstFullOctave) * whiteKeysPerOctave + whiteIndexInOctave[pc]; // A0, B0, then C1 = 2
            const float whiteWidth = (float) keyboard.getWidth() / totalWhiteKeys;
            const int keyX = (int) (whiteIndex * whiteWidth);
            return layer.getScreenBounds().withX (layer.getScreenX() + keyX + columnInset).withWidth ((int) whiteWidth - columnTrim);
        };
        const int lit = litThreshold, dark = darkThreshold;

        beginTest (heldNotesAreDrawnInTheirOwn);
        {
            layer.noteOnReceived (middleC);
            layer.noteOnReceived (noteE4);
            pumpMessages (notesSettleMs);
            expectGreaterThan (brightestOnScreen (column (middleC)), lit);
            expectGreaterThan (brightestOnScreen (column (noteE4)), lit);
            expectLessThan (brightestOnScreen (column (untouchedC7)), dark);
        }

        beginTest (releasedNotesRiseOffTheScreenAnd);
        {
            layer.noteOffReceived (middleC);
            layer.noteOffReceived (noteE4);
            pumpMessages (riseOffMs);
            expectLessThan (brightestOnScreen (layer.getScreenBounds()), dark);
        }

        beginTest (drawingSurvivesTheHideShowCycleThat);
        {
            layer.setVisible (false);
            pumpMessages (hideMs);
            layer.setVisible (true);
            layer.resetState();
            pumpMessages (notesSettleMs);
            layer.noteOnReceived (noteC5);
            pumpMessages (notesSettleMs);
            expectGreaterThan (brightestOnScreen (column (noteC5)), lit);
        }

        beginTest (resetStateActiveNotesReleasesHeldNotesResetStateClearsAt);
        {
            layer.resetStateActiveNotes();
            pumpMessages (riseOffMs);
            expectLessThan (brightestOnScreen (layer.getScreenBounds()), dark);

            layer.noteOnReceived (noteC3);
            pumpMessages (noteAppearsMs);
            expectGreaterThan (brightestOnScreen (column (noteC3)), lit);
            layer.resetState();
            pumpMessages (clearMs);
            expectLessThan (brightestOnScreen (layer.getScreenBounds()), dark);
        }

        beginTest (heldHiddenStaysHidden);
        {
            layer.setVisible (false);
            layer.setHeldHidden (true);
            layer.noteOnReceived (noteC3);
            pumpMessages (noteAppearsMs);
            expect (! layer.isVisible());
            layer.noteOffReceived (noteC3);

            layer.setHeldHidden (false);
            layer.noteOnReceived (noteC3);
            pumpMessages (noteAppearsMs);
            expect (layer.isVisible());
            layer.resetState();
            pumpMessages (clearMs);
        }

        // Owner (2026-11-27): the sparks "can't even be seen". Tap the same key with particles off, then on: the note
        // lights the same pixels both times, so what the particles add is the difference.
        auto litAboveKeyAfterTap = [&] (bool particles, int style)
        {
            layer.resetState();
            layer.setSpawnParticleState (particles);
            layer.setParticleStyle (style);
            pumpMessages (clearMs);
            layer.noteOnReceived (middleC);
            pumpMessages (tapMs);
            layer.noteOffReceived (middleC);
            pumpMessages (particlesRiseMs);
            const auto key = column (middleC);
            const auto whiteWidth = keyboard.getWidth() / (int) totalWhiteKeys;
            const juce::Rectangle<int> above (key.getX() - keysAround * whiteWidth, layer.getScreenY() + layerHeight - areaAboveKey,
                                              (keysAround + keysAround + midiOctaveOffset) * whiteWidth, areaAboveKey);
            const auto lit = litPixelsOnScreen (above, glowThreshold);
            pumpMessages (particlesGoneMs);
            return lit;
        };

        // What the particles add over the note alone, summed over several taps: each tap's sparks are random and
        // the note-only count drifts with frame timing (measured 2026-11-27: one spark tap adds 197-429 px).
        // The colour is the dimmest one the layer can start with, so the check holds for every launch.
        auto dimmestDefault = juce::Colour::fromHSV (DimmestDefaultParticle::hue, DimmestDefaultParticle::saturation,
                                                     DimmestDefaultParticle::brightness, DimmestDefaultParticle::alpha);
        layer.setColourParticle (dimmestDefault);
        auto extraLitAboveKey = [&] (int style, int taps)
        {
            int extra {};
            for (int tap {}; tap < taps; ++tap)
            {
                const auto withoutParticles = litAboveKeyAfterTap (false, style);
                extra += litAboveKeyAfterTap (true, style) - withoutParticles;
            }
            return extra;
        };

        beginTest (sparksAreVisible);
        {
            expectGreaterThan (extraLitAboveKey (ParticleStyles::sparks, sparkTaps), sparkTaps * minSparkPixelsPerTap);
        }

        beginTest (wispsAreVisible);
        {
            expectGreaterThan (extraLitAboveKey (ParticleStyles::wisps, wispTaps), wispTaps * minWispPixelsPerTap);
        }

        host.removeFromDesktop();
        host.removeAllChildren();
        pumpMessages (closeMs);
    }
};

static NoteLayerIntegrationTest noteLayerIntegrationTest;

#endif
