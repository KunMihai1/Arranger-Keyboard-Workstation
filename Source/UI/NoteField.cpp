#include "NoteField.h"

#include <algorithm>

namespace
{
    constexpr float fieldTop          = 0.0f;
    constexpr float transparent       = 0.0f;
    constexpr float minVisibleHeight  = 0.1f;   // px; a note squeezed below this is gone
}

bool NoteField::noteOn (int midiNote, float left, float width)
{
    if (heldNotes.count (midiNote) != std::size_t {})
        return false;

    Note note;
    note.left = left;
    note.width = width;
    note.height = startHeight;
    note.top = fieldHeight - startHeight;
    heldNotes[midiNote] = note;
    return true;
}

void NoteField::noteOff (int midiNote)
{
    auto it = heldNotes.find (midiNote);
    if (it == heldNotes.end())
        return;

    it->second.alpha = opaque;
    fallingNotes.push_back (it->second);
    heldNotes.erase (it);
}

void NoteField::releaseAll()
{
    for (auto& held : heldNotes)
    {
        held.second.alpha = opaque;
        fallingNotes.push_back (held.second);
    }
    heldNotes.clear();
}

void NoteField::clear()
{
    heldNotes.clear();
    fallingNotes.clear();
}

void NoteField::advance (float seconds)
{
    const float rise = riseSpeed * seconds;

    // Held: the top rises, the bottom stays put, never taller than the field.
    for (auto& held : heldNotes)
    {
        auto& note = held.second;
        note.top -= rise;
        note.height += rise;

        if (note.height > fieldHeight)
        {
            const float bottom = note.top + note.height;
            note.height = fieldHeight;
            note.top = bottom - note.height;
        }
    }

    // Released: the whole note rises; against the top edge it shrinks, and fades over its tail.
    for (auto it = fallingNotes.begin(); it != fallingNotes.end();)
    {
        auto& note = *it;
        note.top -= rise;

        if (note.top < fieldTop)
        {
            note.height += note.top;
            note.top = fieldTop;
        }

        const float tailY = note.top + note.height;
        if (tailY < fadeZone)
            note.alpha = std::max (transparent, tailY / fadeZone);

        if (tailY <= fieldTop || note.height <= minVisibleHeight || note.alpha <= transparent)
            it = fallingNotes.erase (it);
        else
            ++it;
    }
}
