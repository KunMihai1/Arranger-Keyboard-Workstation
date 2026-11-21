#include "NoteField.h"

#include <algorithm>

bool NoteField::noteOn (int midiNote, float x, float width)
{
    if (heldNotes.count (midiNote) != 0)
        return false;

    Note n;
    n.x = x;
    n.width = width;
    n.height = startHeight;
    n.y = fieldHeight - startHeight;
    heldNotes[midiNote] = n;
    return true;
}

void NoteField::noteOff (int midiNote)
{
    auto it = heldNotes.find (midiNote);
    if (it == heldNotes.end())
        return;

    it->second.alpha = 1.0f;
    fallingNotes.push_back (it->second);
    heldNotes.erase (it);
}

void NoteField::releaseAll()
{
    for (auto& held : heldNotes)
    {
        held.second.alpha = 1.0f;
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
        auto& n = held.second;
        n.y -= rise;
        n.height += rise;

        if (n.height > fieldHeight)
        {
            const float bottom = n.y + n.height;
            n.height = fieldHeight;
            n.y = bottom - n.height;
        }
    }

    // Released: the whole note rises; against the top edge it shrinks, and fades over its tail.
    for (auto it = fallingNotes.begin(); it != fallingNotes.end();)
    {
        auto& n = *it;
        n.y -= rise;

        if (n.y < 0.0f)
        {
            n.height += n.y;
            n.y = 0.0f;
        }

        const float tailY = n.y + n.height;
        if (tailY < fadeZone)
            n.alpha = std::max (0.0f, tailY / fadeZone);

        if (tailY <= 0.0f || n.height <= 0.1f || n.alpha <= 0.0f)
            it = fallingNotes.erase (it);
        else
            ++it;
    }
}
