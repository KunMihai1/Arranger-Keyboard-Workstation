#include "ChordDetector.h"
#include <algorithm>
#include <array>
#include <iterator>

namespace
{
    using MusicFacts::semitonesPerOctave;

    constexpr int silentVelocity        = 0;   // a note-on at velocity 0 is a note-off
    constexpr int fullKeyboardMinNotes  = 3;   // Full-Keyboard mode needs at least a triad
    constexpr int triadNotes            = 3;
    constexpr int seventhChordNotes     = 4;
    constexpr int majorThird            = 4;   // semitones above the root
    constexpr int minorThird            = 3;
    constexpr int noCountYet            = -1;
    constexpr int blackKeyPitchClasses[] = { 1, 3, 6, 8, 10 };   // C#, D#, F#, G#, A#

    int pitchClassOf (int noteNumber) { return ((noteNumber % semitonesPerOctave) + semitonesPerOctave) % semitonesPerOctave; }

    struct Template { ChordQuality quality; std::vector<int> intervals; };

    // Richer (4-note) templates first so a 4-note chord wins over its 3-note subset.
    const std::vector<Template>& templates()
    {
        static const std::vector<Template> chordTemplates = {
            { ChordQuality::Maj7,    chordIntervals (ChordQuality::Maj7)    },
            { ChordQuality::Dom7,    chordIntervals (ChordQuality::Dom7)    },
            { ChordQuality::Min7,    chordIntervals (ChordQuality::Min7)    },
            { ChordQuality::HalfDim, chordIntervals (ChordQuality::HalfDim) },
            { ChordQuality::Maj,     chordIntervals (ChordQuality::Maj)     },
            { ChordQuality::Min,     chordIntervals (ChordQuality::Min)     },
            { ChordQuality::Dim,     chordIntervals (ChordQuality::Dim)     },
            { ChordQuality::Aug,     chordIntervals (ChordQuality::Aug)     },
            { ChordQuality::Sus2,    chordIntervals (ChordQuality::Sus2)    },
            { ChordQuality::Sus4,    chordIntervals (ChordQuality::Sus4)    },
        };
        return chordTemplates;
    }

    bool matches (const std::set<int>& pitchClasses, int root, const std::vector<int>& intervals)
    {
        if ((int) pitchClasses.size() != (int) intervals.size()) return false;
        for (int interval : intervals)
            if (pitchClasses.find ((root + interval) % semitonesPerOctave) == pitchClasses.end()) return false;
        return true;
    }
}

void ChordDetector::noteOn  (int noteNumber) { heldNotes.insert (noteNumber); recompute(); }
void ChordDetector::noteOff (int noteNumber) { heldNotes.erase  (noteNumber); recompute(); }

void ChordDetector::reset()
{
    heldNotes.clear();
    recognized = ArrangerChord{};
}

ArrangerChord ChordDetector::recognizeSet (const std::set<int>& notes)
{
    if (notes.empty()) return {};

    std::set<int> pitchClasses;
    for (int noteNumber : notes) pitchClasses.insert (noteNumber % semitonesPerOctave);
    const int lowestPc = (*notes.begin()) % semitonesPerOctave;   // std::set is ascending

    // Some pitch-class sets name two chords that are inversions of each other (e.g. C-F-G is both
    // Csus4 and Fsus2). Prefer the interpretation whose root IS the lowest (bass) note â€” the Fingered
    // convention. If no match is rooted on the bass (a true inversion like E-G-C = C major), fall back
    // to the highest-priority template match.
    ArrangerChord firstMatch { PitchClasses::none, ChordQuality::None, lowestPc };
    for (const auto& chordTemplate : templates())
        for (int root {}; root < semitonesPerOctave; ++root)
            if (matches (pitchClasses, root, chordTemplate.intervals))
            {
                if (root == lowestPc)
                    return { root, chordTemplate.quality, lowestPc };
                if (! firstMatch.isValid())
                    firstMatch = { root, chordTemplate.quality, lowestPc };
            }

    return firstMatch;   // invalid if nothing matched
}

ArrangerChord ChordDetector::recognizeSingleFinger (const std::set<int>& notes)
{
    // Korg "One Finger" / Yamaha "Single Finger": the highest held note is the root; keys to its left
    // pick the quality by colour. root alone = Major; + white-left = Dom7; + black-left = Min;
    // + white & black-left = Min7.
    if (notes.empty()) return {};

    const int root   = *notes.rbegin();          // highest held note is the root
    const int rootPc = pitchClassOf (root);

    auto isBlack = [] (int noteNumber)
    {
        const int pitchClass = pitchClassOf (noteNumber);
        return std::find (std::begin (blackKeyPitchClasses), std::end (blackKeyPitchClasses), pitchClass)
               != std::end (blackKeyPitchClasses);
    };

    bool hasWhiteBelow = false, hasBlackBelow = false;
    for (int noteNumber : notes)
        if (noteNumber < root) { (isBlack (noteNumber) ? hasBlackBelow : hasWhiteBelow) = true; }

    const ChordQuality quality = (hasWhiteBelow && hasBlackBelow) ? ChordQuality::Min7
                         : hasBlackBelow                    ? ChordQuality::Min
                         : hasWhiteBelow                    ? ChordQuality::Dom7
                         :                                    ChordQuality::Maj;

    return { rootPc, quality, rootPc };   // bassNote = root (no slash in single-finger)
}

void ChordDetector::recompute()
{
    if (mode != ChordMode::FullKeyboard)
    {
        // Split-zone modes: Fingered matches every held note as a chord tone; Single-Finger uses the
        // Korg one-finger convention (1-2 keys). Empty -> no chord.
        recognized = (mode == ChordMode::SingleFinger)
                       ? recognizeSingleFinger (heldNotes)
                       : recognizeSet (heldNotes);
        return;
    }

    // Full-Keyboard mode: only update on >=3 notes forming a known chord; otherwise hold the last
    // recognized chord so melody notes don't lurch the accompaniment.
    if ((int) heldNotes.size() < fullKeyboardMinNotes)
        return;   // keep the previously recognized chord (hysteresis / hold)

    // Favour the bass hand: try the lowest 3 (the core triad), then the lowest 4 (a bass 7th), then
    // the full set as a last resort. The first valid match wins, so a melody played on top is ignored.
    // Pure and instant (no debounce); stability comes from order + hysteresis, not from delay.
    auto lowest = [this] (int take)
    {
        std::set<int> lowestNotes;
        for (int noteNumber : heldNotes) { lowestNotes.insert (noteNumber); if ((int) lowestNotes.size() == take) break; }
        return lowestNotes;
    };

    for (int take : { triadNotes, seventhChordNotes })
    {
        ArrangerChord candidate = recognizeSet (lowest (take));
        if (candidate.isValid()) { recognized = candidate; return; }
    }
    ArrangerChord fullSetChord = recognizeSet (heldNotes);
    if (fullSetChord.isValid())
        recognized = fullSetChord;
    // else: hold the previously recognized chord (hysteresis)
}

ArrangerChord detectKeyFromEvents (const std::vector<TimedBeatEvent>& events)
{
    std::array<int, semitonesPerOctave> notesPerPitchClass {};
    for (const auto& event : events)
        if (event.message.isNoteOn() && event.message.getVelocity() > silentVelocity)
            notesPerPitchClass[event.message.getNoteNumber() % semitonesPerOctave]++;

    int total {}, root {}, best = noCountYet;
    for (int pitchClass {}; pitchClass < semitonesPerOctave; ++pitchClass)
    {
        total += notesPerPitchClass[pitchClass];
        if (notesPerPitchClass[pitchClass] > best) { best = notesPerPitchClass[pitchClass]; root = pitchClass; }
    }
    if (total == int {}) return { PitchClasses::lowest, ChordQuality::Maj, PitchClasses::lowest };   // fallback C major

    const int majThird = notesPerPitchClass[(root + majorThird) % semitonesPerOctave];
    const int minThird = notesPerPitchClass[(root + minorThird) % semitonesPerOctave];
    const ChordQuality quality = (minThird > majThird) ? ChordQuality::Min : ChordQuality::Maj;
    return { root, quality, root };
}
