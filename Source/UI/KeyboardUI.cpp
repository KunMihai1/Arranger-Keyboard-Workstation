/*
  ==============================================================================

    keyboardUI.cpp
    Created: 26 Mar 2025 9:45:11pm
    Author:  Kisuke

  ==============================================================================
*/

#include "KeyboardUI.h"

namespace
{
    // An 88-key piano: A0 (MIDI 21) to C8, 52 white keys.
    namespace KeyLayout
    {
        constexpr float totalWhiteKeys        = 52.0f;
        constexpr int   lowestPianoNote       = 21;      // A0
        constexpr int   aSharp0               = 22;      // the lowest black key
        constexpr int   whiteKeysBeforeFirstC = 2;       // A0 and B0
        constexpr int   whiteKeysPerOctave    = 7;
        constexpr int   blackKeysPerOctave    = 5;
        // Semitones above A0 that the key-walk loops recognise.
        constexpr int   bNatural0             = 2;
        constexpr int   cSharp1               = 4;
        constexpr int   e1                    = 7;
        constexpr int   wholeTone             = 2;
        constexpr int   oneStep               = 1;
        constexpr int   twoSteps              = 2;
        constexpr int   threeSteps            = 3;
        constexpr int   parityDivisor         = 2;
        constexpr int   even                  = 0;
        constexpr int   odd                   = 1;
        constexpr int   firstWhiteKey         = 0;
        constexpr int   previousKey           = 1;
        constexpr float keyboardLeft          = 0.0f;
        constexpr int   keyTop                = 0;
        constexpr float blackKeyWidthRatio    = 0.6f;
        constexpr float blackKeyHeightRatio   = 0.6f;
        constexpr float half                  = 0.5f;
        // Where each black key of an octave sits, in white-key widths from the octave's C.
        constexpr float blackKeyOffsets[]     = { 0.7f, 1.7f, 3.2f, 4.2f, 5.2f };
        constexpr float shade                 = 0.5f;    // gradient: brighter at the top, darker at the bottom
        constexpr int   gradientX             = 0;
        constexpr int   gradientTop           = 0;
        constexpr float outOfRangeAlpha       = 0.5f;    // keys outside the playable range
        constexpr float opaque                = 1.0f;
        constexpr float noteNameAlpha         = 0.8f;
        constexpr float noteNameSizeRatio     = 0.12f;
        constexpr float noteNameAreaRatio     = 1.6f;
        constexpr int   singleLine            = 1;
    }
}

KeyboardUI::KeyboardUI(MidiHandler& midiHandler) : midiHandler{ midiHandler }
{
    setSize(getWidth(), getHeight());
    //midiHandler.addListener(this);
}

KeyboardUI::~KeyboardUI()
{
    //midiHandler.removeListener(this);
}

void KeyboardUI::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    paintKeyboardB(g);
}

void KeyboardUI::resized()
{
    using namespace KeyLayout;
    float rawWidth = static_cast<float>(getWidth()) / totalWhiteKeys;
    float whiteKeyWidth = rawWidth;
    float keyHeight = static_cast<float>(getHeight());

    juce::Array<float> whiteKeyPositions;
    // noteAccount: semitones above A0 of the key being placed; refresh counts the steps since the last
    // half step, which is how the loops find E-F and B-C (white keys a semitone apart).
    int noteAccount {};
    int refresh {};


    for (int whiteIndex {}; whiteIndex < whiteKeysBeforeFirstC; whiteIndex++)
    {
        float keyX = (whiteIndex == firstWhiteKey) ? keyboardLeft : whiteKeyPositions[whiteIndex - previousKey] + whiteKeyWidth;
        whiteKeyPositions.add(keyX);

        keys[lowestPianoNote + noteAccount].bounds = juce::Rectangle<int>(static_cast<int>(keyX), keyTop,
            static_cast<int>(whiteKeyWidth), static_cast<int>(keyHeight));
        keys[lowestPianoNote + noteAccount].type = KeyColourNames::white;
        keys[lowestPianoNote + noteAccount].isActive = false;

        if ((noteAccount == bNatural0 && refresh == oneStep))
        {
            noteAccount--;
            refresh = {};
        }
        else refresh++;

        noteAccount += wholeTone;
    }


    float blackKeyWidth = whiteKeyWidth * blackKeyWidthRatio;
    float blackKeyHeight = keyHeight * blackKeyHeightRatio;
    float firstBlackKeyX = whiteKeyPositions[firstWhiteKey] + whiteKeyWidth - blackKeyWidth * half;

    keys[aSharp0].bounds = juce::Rectangle<int>(static_cast<int>(firstBlackKeyX), keyTop,
        static_cast<int>(blackKeyWidth), static_cast<int>(blackKeyHeight));
    keys[aSharp0].type = KeyColourNames::black;
    keys[aSharp0].isActive = false;

    for (int whiteIndex = whiteKeysBeforeFirstC; whiteIndex < totalWhiteKeys; whiteIndex++)
    {
        float keyX = whiteKeyPositions[whiteIndex - previousKey] + whiteKeyWidth;
        whiteKeyPositions.add(keyX);

        keys[lowestPianoNote + noteAccount].bounds = juce::Rectangle<int>(static_cast<int>(keyX), keyTop,
            static_cast<int>(whiteKeyWidth), static_cast<int>(keyHeight));
        keys[lowestPianoNote + noteAccount].type = KeyColourNames::white;
        keys[lowestPianoNote + noteAccount].isActive = false;

        if ((noteAccount == e1 && refresh == twoSteps) || (refresh == threeSteps) || (refresh == twoSteps && noteAccount % parityDivisor == odd))
        {
            noteAccount--;
            refresh = {};
        }
        else refresh++;

        noteAccount += wholeTone;
    }


    int octaveCount = totalWhiteKeys / whiteKeysPerOctave;
    noteAccount = cSharp1;
    refresh = {};

    for (int octave {}; octave < octaveCount; octave++)
    {
        int octaveStartIndex = whiteKeysBeforeFirstC + octave * whiteKeysPerOctave;
        for (int blackIndex {}; blackIndex < blackKeysPerOctave; blackIndex++)
        {
            int whiteIndex = octaveStartIndex + static_cast<int>(blackKeyOffsets[blackIndex]);
            if (whiteIndex < whiteKeyPositions.size() - previousKey)
            {
                float keyX = whiteKeyPositions[whiteIndex] + whiteKeyWidth - (blackKeyWidth * half);
                keys[lowestPianoNote + noteAccount].bounds = juce::Rectangle<int>(static_cast<int>(keyX), keyTop,
                    static_cast<int>(blackKeyWidth),
                    static_cast<int>(blackKeyHeight));
                keys[lowestPianoNote + noteAccount].type = KeyColourNames::black;
                keys[lowestPianoNote + noteAccount].isActive = false;
            }

            if ((refresh == oneStep && noteAccount % parityDivisor == even) || (refresh == threeSteps) || (refresh == twoSteps && noteAccount % parityDivisor == odd))
            {
                noteAccount++;
                refresh = {};
            }
            else refresh++;

            noteAccount += wholeTone;
        }
    }
}


void KeyboardUI::set_min_and_max(const int min, const int max)
{
    this->min_draw = min;
    this->max_draw = max;
}

int KeyboardUI::get_min()
{
    return this->min_draw;
}

int KeyboardUI::get_max()
{
    return this->max_draw;
}


void KeyboardUI::resetStateActiveNotes()
{
    for (auto& keyPair : keys)
    {
        keyPair.second.isActive = false;
    }

    repaint();
}

void KeyboardUI::resetStateActiveNotesNoExplicitRepaint()
{
    for (auto& keyPair : keys)
    {
        keyPair.second.isActive = false;
    }
}

void KeyboardUI::noteOnReceived(int midiNote)
{
    juce::MessageManager::callAsync([this, midiNote]
        {
            keys[midiNote].isActive = true;
            repaint(keys[midiNote].bounds);
        });
}

void KeyboardUI::noteOffReceived(int midiNote)
{
    juce::MessageManager::callAsync([this, midiNote]
        {
            keys[midiNote].isActive = false;
            repaint(keys[midiNote].bounds);
        });
}

void KeyboardUI::setIsDrawn(bool state)
{
    this->isDrawn = state;
}

void KeyboardUI::setAnnotationState(bool newState)
{
    this->annotationState = newState;
}

void KeyboardUI::paintKeyboardB(juce::Graphics& g)
{
    for (const auto& [key, MidiNote] : keys)
    {
        
        if (MidiNote.isActive)
        {
            juce::ColourGradient greenGradient(
                juce::Colours::green.brighter(KeyLayout::shade), KeyLayout::gradientX, KeyLayout::gradientTop,
                juce::Colours::green.darker(KeyLayout::shade), KeyLayout::gradientX, MidiNote.bounds.getHeight(), false);
            g.setGradientFill(greenGradient);
            g.fillRect(MidiNote.bounds);
        }
        else
        {
            if (MidiNote.type == KeyColourNames::white)
            {
                if (key < min_draw || key > max_draw)
                    g.setColour(juce::Colours::white.withAlpha(KeyLayout::outOfRangeAlpha));
                else
                    g.setColour(juce::Colours::white);

                g.fillRect(MidiNote.bounds);
            }
            else 
            {
                juce::ColourGradient blackGradient;
                if (key < min_draw || key > max_draw)
                {
                    blackGradient = juce::ColourGradient(
                        juce::Colours::black.withAlpha(KeyLayout::opaque).brighter(KeyLayout::shade), KeyLayout::gradientX, KeyLayout::gradientTop,
                        juce::Colours::black.withAlpha(KeyLayout::opaque).darker(KeyLayout::shade), KeyLayout::gradientX,
                        MidiNote.bounds.getHeight(), false);
                }
                else
                {
                    blackGradient = juce::ColourGradient(
                        juce::Colours::black.brighter(KeyLayout::shade), KeyLayout::gradientX, KeyLayout::gradientTop,
                        juce::Colours::black.darker(KeyLayout::shade), KeyLayout::gradientX,
                        MidiNote.bounds.getHeight(), false);
                }

                g.setGradientFill(blackGradient);
                g.fillRect(MidiNote.bounds);
            }
        }

        if (annotationState && MidiNote.type == KeyColourNames::white)
        {
            juce::String noteName = MapHelper::intToStringNote(key);
            g.setColour(juce::Colours::black.withAlpha(KeyLayout::noteNameAlpha));

            float fontSize = MidiNote.bounds.getHeight() * KeyLayout::noteNameSizeRatio;
            g.setFont(fontSize);

            juce::Rectangle<int> textArea =
                MidiNote.bounds.withTop(
                    MidiNote.bounds.getBottom() - static_cast<int>(fontSize * KeyLayout::noteNameAreaRatio)
                );

            g.drawFittedText(noteName, textArea, juce::Justification::centred, KeyLayout::singleLine);
        }

        
        g.setColour(juce::Colours::black);
        g.drawRect(MidiNote.bounds);
    }
}
