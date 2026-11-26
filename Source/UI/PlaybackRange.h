#pragma once
#include <JuceHeader.h>
#include "PlayBackSettingsCustomComponent.h"

/*  The playing range, split and the PC keyboard's octave (Z / X). The saved settings are the home-octave values;
    what the Voices page shows and the MIDI handler uses is those values moved by the keyboard's octave shift. An edit
    made while shifted is saved moved back home, only for the field that changed, so shifting never changes what is
    saved (it used to: a shifted value was saved folded into the middle-C octave, A3 coming back as A4). Pure. */
namespace PlaybackRange
{
    inline constexpr int middleC          = 60;    // the PC keyboard's home: it starts at middle C (keyListener)
    inline constexpr int lowestPianoNote  = 21;    // A0
    inline constexpr int highestPianoNote = 108;   // C8

    /** How far the playing keyboard is from home: the PC keyboard (Z / X) by where it starts against middle C; a
        hardware keyboard, whatever its size, is never shifted (0). */
    int octaveShift (int lowestKey, bool pcKeyboard);

    /** The saved settings as they apply now: each set note moved by the shift and kept on the piano; "none" stays. */
    PlayBackSettings shown (const PlayBackSettings& saved, int shift);

    /** After an edit on screen: the saved settings with each field that differs from what was shown taken from the
        edit, moved back home ("none" stays none). Untouched fields keep their saved value, even if shown clamped. */
    PlayBackSettings savedAfterEdit (const PlayBackSettings& saved, const PlayBackSettings& edited, int shift);
}
