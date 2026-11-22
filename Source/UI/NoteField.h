#pragma once

#include <map>
#include <vector>

/**
    The falling-notes animation model behind NoteLayer: pure state + time, no GUI, no GL.

    NoteLayer owns one of these on its OpenGL render thread and draws whatever it holds.
    Kept separate so the motion rules are unit-tested (tests/unit/test_note_field.cpp).
    Coordinates are NoteLayer-local logical pixels, y pointing down.
*/
class NoteField
{
public:
    static constexpr float opaque = 1.0f;

    struct Note
    {
        float left {}, width {};        // from the key's bounds at note-on
        float top {}, height {};        // top edge and height
        float alpha = opaque;           // fades to 0 as a released note leaves the top
    };

    static constexpr float riseSpeed   = 400.0f;  // px per second
    static constexpr float startHeight = 10.0f;   // px, a fresh note
    static constexpr float fadeZone    = 80.0f;   // px, released notes fade over their last 80 px

    /** Field height in px; new notes spawn at the bottom and held notes never grow taller. */
    void setHeight (float newHeight) noexcept   { fieldHeight = newHeight; }

    /** Starts a held note. Returns false (and changes nothing) if that note is already held. */
    bool noteOn (int midiNote, float left, float width);

    /** Releases a held note; it keeps rising and fades out. Unknown notes are ignored. */
    void noteOff (int midiNote);

    /** Releases every held note. */
    void releaseAll();

    /** Removes everything immediately. */
    void clear();

    /** Moves the animation forward by `seconds`. */
    void advance (float seconds);

    bool isIdle() const noexcept                         { return heldNotes.empty() && fallingNotes.empty(); }
    const std::map<int, Note>& held() const noexcept     { return heldNotes; }
    const std::vector<Note>& falling() const noexcept    { return fallingNotes; }

private:
    float fieldHeight {};
    std::map<int, Note> heldNotes;
    std::vector<Note> fallingNotes;
};
