/*
  ==============================================================================

    keyboardUI.cpp
    Created: 26 Mar 2025 9:45:11pm
    Author:  Kisuke

  ==============================================================================
*/

#include "KeyboardUI.h"
#include "PianoKeyShapes.h"
#include "AppColours.h"

namespace
{
    // Board StageRevised, keys region (1200 x 140): lengths in board px, multiplied by scale = width / 1200.
    namespace KeyLook
    {
        constexpr float boardWidth          = 1200.0f;
        constexpr float whiteCorner         = 3.0f;
        constexpr float blackCorner         = 2.0f;
        constexpr float whiteLip            = 6.0f;
        constexpr float pressedLip          = 3.0f;
        constexpr float edgeWidth           = 1.0f;    // white key's right edge, black key's bevels
        constexpr float pressedShadeDepth   = 8.0f;    // inset 0 5px 6px -3px: dark at the top, gone by ~8 px
        constexpr float shadowBlur          = 4.0f;
        constexpr float shadowRight         = 1.0f;
        constexpr float shadowDown          = 3.0f;
        constexpr int   shadowSafety        = 1;       // a blur reaches one px past its radius
        constexpr int   bothSides           = 2;
        constexpr int   semitone            = 1;       // A0 + 1 = A#0, the lowest black key

        // Gradient stops (fractions of the key's height)
        constexpr double top                = 0.0;
        constexpr double bottom             = 1.0;
        constexpr double whiteUpper         = 0.12;
        constexpr double whiteLower         = 0.80;
        constexpr double pressedWhiteUpper  = 0.14;
        constexpr double pressedWhiteLower  = 0.85;
        constexpr double blackBody          = 0.82;
        constexpr double blackSlope         = 0.84;
        constexpr double pressedBlackBody   = 0.88;
        constexpr double pressedBlackSlope  = 0.90;

        constexpr float outOfRangeAlpha     = 0.5f;    // white keys outside the device's range (as before)
        constexpr float noteNameAlpha       = 0.8f;
        constexpr float noteNameSizeRatio   = 0.12f;
        constexpr float noteNameAreaRatio   = 1.6f;
        constexpr int   singleLine          = 1;
        constexpr float originX             = 0.0f;
        constexpr float originY             = 0.0f;
        constexpr bool  linear              = false;   // ColourGradient isRadial
        constexpr bool  square              = false;
        constexpr bool  rounded             = true;
    }

    /** The shape's left and right edges on whole pixels, so the 1-px gaps between keys stay crisp (as a browser
        draws the board) instead of turning grey from anti-aliasing. */
    juce::Rectangle<float> snappedToPixels (juce::Rectangle<float> shape)
    {
        return juce::Rectangle<float>::leftTopRightBottom (std::round (shape.getX()), shape.getY(),
                                                           std::round (shape.getRight()), std::round (shape.getBottom()));
    }

    juce::Path keyOutline (juce::Rectangle<float> shape, float corner)
    {
        juce::Path outline;
        outline.addRoundedRectangle (shape.getX(), shape.getY(), shape.getWidth(), shape.getHeight(), corner, corner,
                                     KeyLook::square, KeyLook::square, KeyLook::rounded, KeyLook::rounded);
        return outline;
    }

    juce::ColourGradient verticalGradient (float height, juce::Colour topColour, juce::Colour bottomColour)
    {
        return juce::ColourGradient (topColour, KeyLook::originX, KeyLook::originY,
                                     bottomColour, KeyLook::originX, height, KeyLook::linear);
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
    g.fillAll(StagePalette::keyBed);
    paintKeyboardB(g);
}

void KeyboardUI::resized()
{
    const auto width  = static_cast<float>(getWidth());
    const auto height = static_cast<float>(getHeight());
    const auto step   = width / static_cast<float>(PianoKeyShapes::whiteKeys);
    keyImageCache.clear();   // the key looks are drawn again at the new size
    blackShadowImage = {};

    for (int midiNote = PianoKeyShapes::lowestNote; midiNote <= PianoKeyShapes::highestNote; ++midiNote)
    {
        auto& key = keys[midiNote];
        key.shape = snappedToPixels(PianoKeyShapes::shapeOf(midiNote, width, height));
        key.isActive = false;

        if (PianoKeyShapes::isBlack(midiNote))
        {
            key.type = KeyColourNames::black;
            key.bounds = key.shape.toNearestInt();
        }
        else
        {
            // The whole step (key + gap): the falling-note column the note layer lines its notes up with.
            key.type = KeyColourNames::white;
            key.bounds = juce::Rectangle<int>(static_cast<int>(PianoKeyShapes::whiteKeysBelow(midiNote) * step), int {},
                                              static_cast<int>(step), static_cast<int>(height));
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

// Called on the MIDI thread; the key is drawn later on the message thread, by which time the keyboard may be gone
// (a note arriving while the app closes), so the message holds a SafePointer, not `this`.
void KeyboardUI::noteOnReceived(int midiNote)
{
    juce::Component::SafePointer<KeyboardUI> keyboard(this);
    juce::MessageManager::callAsync([keyboard, midiNote] { if (keyboard != nullptr) keyboard->setKeyDown(midiNote, true); });
}

void KeyboardUI::noteOffReceived(int midiNote)
{
    juce::Component::SafePointer<KeyboardUI> keyboard(this);
    juce::MessageManager::callAsync([keyboard, midiNote] { if (keyboard != nullptr) keyboard->setKeyDown(midiNote, false); });
}

void KeyboardUI::setKeyDown(int midiNote, bool isDown)
{
    keys[midiNote].isActive = isDown;
    repaint(areaOf(midiNote));
}

juce::Rectangle<int> KeyboardUI::areaOf(int midiNote) const
{
    const auto key = keys.find(midiNote);
    // A white key's bounds (its step, the falling-note column) can end a pixel before its drawn right edge.
    return key != keys.end() ? key->second.bounds.getUnion(key->second.shape.getSmallestIntegerContainer()) : juce::Rectangle<int>();
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
    // Key bed (filled by paint), the white keys, the black keys' shadows, then the black keys. Each look is drawn
    // once per size into an image and copied per key: drawing the materials for every key on every paint took
    // 8.75 ms at 1920 x 224 (Debug). Only keys inside the area being repainted are drawn.
    const auto scale = static_cast<float>(getWidth()) / KeyLook::boardWidth;
    const auto clip  = g.getClipBounds();

    for (int midiNote = PianoKeyShapes::lowestNote; midiNote <= PianoKeyShapes::highestNote; ++midiNote)
    {
        const auto& key = keys[midiNote];
        const auto area = key.shape.toNearestInt();   // the shape is already on whole pixels
        if (PianoKeyShapes::isBlack(midiNote) || ! area.intersects(clip))
            continue;

        const auto look = key.isActive ? KeyLookKind::whitePressed
                        : (midiNote < min_draw || midiNote > max_draw) ? KeyLookKind::whiteDimmed
                                                                         : KeyLookKind::white;
        g.drawImageAt(keyImage(look, area.getWidth(), area.getHeight(), scale), area.getX(), area.getY());

        if (annotationState)
            paintNoteName(g, midiNote);
    }

    const auto& shadow = blackKeyShadow(scale);
    const auto shadowOffset = juce::Point<int>(juce::roundToInt(KeyLook::shadowRight * scale) - blackShadowMargin,
                                               juce::roundToInt(KeyLook::shadowDown * scale) - blackShadowMargin);

    for (int midiNote = PianoKeyShapes::lowestNote; midiNote <= PianoKeyShapes::highestNote; ++midiNote)
    {
        if (! PianoKeyShapes::isBlack(midiNote))
            continue;

        const auto shadowAt = keys[midiNote].shape.getPosition().toInt() + shadowOffset;
        if (shadow.getBounds().withPosition(shadowAt).intersects(clip))
            g.drawImageAt(shadow, shadowAt.x, shadowAt.y);
    }

    for (int midiNote = PianoKeyShapes::lowestNote; midiNote <= PianoKeyShapes::highestNote; ++midiNote)
    {
        const auto& key = keys[midiNote];
        const auto area = key.shape.toNearestInt();
        if (! PianoKeyShapes::isBlack(midiNote) || ! area.intersects(clip))
            continue;

        const auto look = key.isActive ? KeyLookKind::blackPressed : KeyLookKind::black;
        g.drawImageAt(keyImage(look, area.getWidth(), area.getHeight(), scale), area.getX(), area.getY());
    }
}

void KeyboardUI::paintNoteName(juce::Graphics& g, int midiNote)
{
    const auto& key = keys[midiNote];
    juce::String noteName = MapHelper::intToStringNote(midiNote);
    g.setColour(juce::Colours::black.withAlpha(KeyLook::noteNameAlpha));

    float fontSize = key.bounds.getHeight() * KeyLook::noteNameSizeRatio;
    g.setFont(fontSize);

    juce::Rectangle<int> textArea =
        key.bounds.withTop(key.bounds.getBottom() - static_cast<int>(fontSize * KeyLook::noteNameAreaRatio));

    g.drawFittedText(noteName, textArea, juce::Justification::centred, KeyLook::singleLine);
}

const juce::Image& KeyboardUI::keyImage(KeyLookKind look, int width, int height, float scale)
{
    auto& image = keyImageCache[{ static_cast<int>(look), width }];
    if (! image.isValid() || image.getHeight() != height)
    {
        image = juce::Image(juce::Image::ARGB, juce::jmax(KeyLook::shadowSafety, width), juce::jmax(KeyLook::shadowSafety, height), true);
        juce::Graphics imageGraphics(image);
        drawKeyLook(imageGraphics, image.getBounds().toFloat(), look, scale);
    }
    return image;
}

void KeyboardUI::drawKeyLook(juce::Graphics& g, juce::Rectangle<float> shape, KeyLookKind look, float scale)
{
    const bool isBlack   = look == KeyLookKind::black || look == KeyLookKind::blackPressed;
    const bool isPressed = look == KeyLookKind::whitePressed || look == KeyLookKind::blackPressed;
    const auto outline   = keyOutline(shape, (isBlack ? KeyLook::blackCorner : KeyLook::whiteCorner) * scale);

    if (isBlack)
    {
        auto gradient = verticalGradient(shape.getBottom(), isPressed ? StagePalette::pressedBlackTop : StagePalette::blackKeyTop,
                                         StagePalette::blackKeySlopeEnd);
        gradient.addColour(isPressed ? KeyLook::pressedBlackBody : KeyLook::blackBody,
                           isPressed ? StagePalette::pressedBlackBody : StagePalette::blackKeyBody);
        gradient.addColour(isPressed ? KeyLook::pressedBlackSlope : KeyLook::blackSlope, StagePalette::blackKeySlopeTop);
        g.setGradientFill(gradient);
        g.fillPath(outline);

        juce::Graphics::ScopedSaveState insideTheKey(g);
        g.reduceClipRegion(outline);
        const auto bevel = KeyLook::edgeWidth * scale;
        g.setColour(StagePalette::blackKeyBevelLeft);
        g.fillRect(shape.withWidth(bevel));
        g.setColour(StagePalette::blackKeyBevelRight);
        g.fillRect(shape.withLeft(shape.getRight() - bevel));
        return;
    }

    auto gradient = isPressed ? verticalGradient(shape.getBottom(), StagePalette::pressedWhiteTop, StagePalette::pressedWhiteBottom)
                              : verticalGradient(shape.getBottom(), StagePalette::whiteKeyTop, StagePalette::whiteKeyBottom);
    gradient.addColour(isPressed ? KeyLook::pressedWhiteUpper : KeyLook::whiteUpper,
                       isPressed ? StagePalette::pressedWhiteUpper : StagePalette::whiteKeyUpper);
    gradient.addColour(isPressed ? KeyLook::pressedWhiteLower : KeyLook::whiteLower,
                       isPressed ? StagePalette::pressedWhiteLower : StagePalette::whiteKeyLower);
    g.setGradientFill(gradient);
    g.fillPath(outline);

    {
        juce::Graphics::ScopedSaveState insideTheKey(g);
        g.reduceClipRegion(outline);

        const auto lipHeight = (isPressed ? KeyLook::pressedLip : KeyLook::whiteLip) * scale;
        g.setColour(isPressed ? StagePalette::pressedWhiteLip : StagePalette::whiteKeyLip);
        g.fillRect(shape.withTop(shape.getBottom() - lipHeight));

        g.setColour(isPressed ? StagePalette::pressedWhiteEdge : StagePalette::whiteKeyEdge);
        g.fillRect(shape.withLeft(shape.getRight() - KeyLook::edgeWidth * scale));

        if (isPressed)
        {
            // Pushed down: the key in front of it shades its top.
            g.setGradientFill(juce::ColourGradient(StagePalette::pressedKeyShade, KeyLook::originX, shape.getY(),
                                                   StagePalette::pressedKeyShade.withAlpha(float {}), KeyLook::originX,
                                                   shape.getY() + KeyLook::pressedShadeDepth * scale, KeyLook::linear));
            g.fillRect(shape.withHeight(KeyLook::pressedShadeDepth * scale));
        }
    }

    if (look == KeyLookKind::whiteDimmed)
    {
        g.setColour(StagePalette::keyBed.withAlpha(KeyLook::outOfRangeAlpha));
        g.fillPath(outline);
    }
}

const juce::Image& KeyboardUI::blackKeyShadow(float scale)
{
    if (blackShadowImage.isValid() && juce::approximatelyEqual(blackShadowScale, scale))
        return blackShadowImage;

    // One black key's shape at this size, blurred once; every black key reuses it.
    const auto key = keys[PianoKeyShapes::lowestNote + KeyLook::semitone].shape;
    const auto blur = juce::jmax(KeyLook::shadowSafety, juce::roundToInt(KeyLook::shadowBlur * scale));
    blackShadowMargin = blur + KeyLook::shadowSafety;
    blackShadowScale = scale;

    const auto keyAtMargin = key.withPosition(static_cast<float>(blackShadowMargin), static_cast<float>(blackShadowMargin));
    blackShadowImage = juce::Image(juce::Image::ARGB,
                                   juce::jmax(KeyLook::shadowSafety, juce::roundToInt(key.getWidth()) + blackShadowMargin * KeyLook::bothSides),
                                   juce::jmax(KeyLook::shadowSafety, juce::roundToInt(key.getHeight()) + blackShadowMargin * KeyLook::bothSides),
                                   true);
    juce::Graphics shadowGraphics(blackShadowImage);
    juce::DropShadow(StagePalette::blackKeyShadow, blur, {}).drawForPath(shadowGraphics, keyOutline(keyAtMargin, KeyLook::blackCorner * scale));
    return blackShadowImage;
}
