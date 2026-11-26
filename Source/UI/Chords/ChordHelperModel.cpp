#include "ChordHelperModel.h"

namespace
{
    constexpr int semitonesPerOctave = 12;
    const juce::String octaveCharacters = "0123456789-";   // "C#4", "C-1": what follows the note's name

    // .mb left = white edge - 4 + this nudge (board: C# -1, D# +1, F# -1, G# 0, A# +1); pitch class -> nudge.
    struct BlackKey
    {
        int pitchClass;
        int nudge;
    };
    // Pitch classes from C.
    constexpr int cSharp = 1;
    constexpr int dSharp = 3;
    constexpr int fSharp = 6;
    constexpr int gSharp = 8;
    constexpr int aSharp = 10;
    constexpr int nudgeLeft  = -1;
    constexpr int noNudge    = 0;
    constexpr int nudgeRight = 1;
    constexpr BlackKey blackKeys[] { { cSharp, nudgeLeft }, { dSharp, nudgeRight }, { fSharp, nudgeLeft },
                                     { gSharp, noNudge }, { aSharp, nudgeRight } };

    const BlackKey* blackKeyFor (int note)
    {
        for (const auto& key : blackKeys)
            if (key.pitchClass == note % semitonesPerOctave)
                return &key;
        return nullptr;
    }

    std::vector<int> inverted (std::vector<int> voicing)
    {
        if (voicing.empty())
            return voicing;
        const auto lowest = voicing.front();
        voicing.erase (voicing.begin());
        voicing.push_back (lowest + semitonesPerOctave);
        return voicing;
    }

    juce::Rectangle<int> roundedEdges (juce::Rectangle<float> area)
    {
        return juce::Rectangle<int>::leftTopRightBottom (juce::roundToInt (area.getX()), juce::roundToInt (area.getY()),
                                                         juce::roundToInt (area.getRight()), juce::roundToInt (area.getBottom()));
    }
}

namespace ChordHelperModel
{
    juce::String noteNames (const std::vector<int>& notes, const juce::String& separator)
    {
        juce::StringArray names;
        for (const auto note : notes)
            names.add (MapHelper::intToStringNote (note).trimCharactersAtEnd (octaveCharacters));
        return names.joinIntoString (separator);
    }

    std::array<std::vector<int>, voicingCount> voicings (const Chord& chord)
    {
        auto rootPosition = ChordHelper::getNotesForChord (chord);
        for (auto& note : rootPosition)
            note -= semitonesPerOctave;   // the board draws C3 to B5; the chord plays from middle C

        std::array<std::vector<int>, voicingCount> shapes;
        shapes.front() = rootPosition;
        for (auto previous = shapes.begin(), next = std::next (previous); next != shapes.end(); ++previous, ++next)
            *next = inverted (*previous);
        return shapes;
    }
}

std::vector<MiniKey> MiniKeyboard::keys (float scale)
{
    using namespace ChordHelperMetrics;
    std::vector<MiniKey> whites, blacks;
    int whiteIndex {};
    for (int note = lowestNote; note <= highestNote; ++note)
    {
        const auto edge = static_cast<float> (whiteIndex * whiteKeyPitch);
        if (const auto* black = blackKeyFor (note))
        {
            const auto left = edge - static_cast<float> (blackKeyShift - black->nudge);
            blacks.push_back ({ juce::Rectangle<float> (left, float {}, static_cast<float> (blackKeyWidth),
                                                        static_cast<float> (blackKeyHeight)) * scale, note, true });
        }
        else
        {
            whites.push_back ({ juce::Rectangle<float> (edge, float {}, static_cast<float> (whiteKeyWidth),
                                                        static_cast<float> (keyboardHeight)) * scale, note, false });
            ++whiteIndex;
        }
    }
    whites.insert (whites.end(), blacks.begin(), blacks.end());
    return whites;
}

ChordHelperRects chordHelperLayout (juce::Rectangle<int> content, float scale)
{
    using namespace ChordHelperMetrics;
    const auto area = content.toFloat();
    const auto scaled = [scale] (int boardLength) { return static_cast<float> (boardLength) * scale; };

    ChordHelperRects rects;
    const auto listWidth = scaled (listColumnWidth);
    rects.search = roundedEdges ({ area.getX(), area.getY(), listWidth, scaled (searchHeight) });
    const auto listTop = area.getY() + scaled (searchHeight + searchGap);
    rects.list = roundedEdges ({ area.getX(), listTop, listWidth, area.getBottom() - listTop });

    const auto rightX = area.getX() + scaled (listColumnWidth + columnGap);
    const auto rightWidth = area.getRight() - rightX;
    rects.name = roundedEdges ({ rightX, area.getY(), rightWidth, scaled (nameHeight) });

    const auto titleY    = area.getY() + scaled (nameHeight + blockGap);
    const auto keyboardY = titleY + scaled (titleHeight + voicingInnerGap);
    const auto notesY    = keyboardY + scaled (keyboardHeight + voicingInnerGap);
    const auto columnWidth = (rightWidth - scaled (voicingGap * voicingGaps)) / static_cast<float> (ChordHelperModel::voicingCount);
    for (size_t column {}; column < rects.keyboard.size(); ++column)
    {
        const auto columnX = rightX + static_cast<float> (column) * (columnWidth + scaled (voicingGap));
        rects.voicingTitle[column] = roundedEdges ({ columnX, titleY, columnWidth, scaled (titleHeight) });
        rects.keyboard[column]     = roundedEdges ({ columnX, keyboardY, columnWidth, scaled (keyboardHeight) });
        rects.voicingNotes[column] = roundedEdges ({ columnX, notesY, columnWidth, scaled (voicingNotesHeight) });
    }

    const auto hintY = notesY + scaled (voicingNotesHeight + blockGap + hintMargin);
    rects.hint = roundedEdges ({ rightX, hintY, rightWidth, scaled (hintHeight) });
    return rects;
}
