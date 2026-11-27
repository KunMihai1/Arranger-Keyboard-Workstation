#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>

/** One Dust particle, in board units (the board is 1200 wide; y grows downwards from the top of the note layer). */
struct DustMote
{
    juce::Point<float> pos;
    juce::Point<float> velocity;   ///< board units per second
    float age {};
    float life {};                 ///< seconds it lives
    float sizeStart {};
    float sizeEnd {};
    float alpha {};
    float seed {};                 ///< its own phase in the flow
    bool  isMote {};               ///< a big soft mote (else a fine speck)
};

/** What to draw for a mote, in layer pixels. */
struct DustSprite
{
    juce::Point<float> centre;
    float radius {};
    float opacity {};   ///< 0..1, times the particle colour's own alpha
};

/*  The Dust particle style of the note layer (board `Dust`, mode "new"; spec 2026-11-27-ui-redesign-dust):
    a burst when a key goes down, a steady stream while it is held, carried by a smooth flow field instead of
    random kicks, fading in and out and growing as it rises. Pure: no GL, no clock; the note layer owns one on its
    render thread and draws spriteOf() for each mote. Never allocates after construction. */
class DustField
{
public:
    static constexpr int maxMotes   = 3200;
    static constexpr int burstCount = 26;

    DustField();

    /** The note layer's size in pixels; the board's numbers are scaled by width / 1200. */
    void setLayerSize (float widthPixels, float heightPixels);

    /** A key went down: a burst from its top edge (key centre and width in layer pixels). */
    void burst (float keyCentreX, float keyWidth, juce::Random& random);

    /** A key is held for `seconds`: 60 motes a second from its top edge (fractions by chance). */
    void emitHeld (float keyCentreX, float keyWidth, float seconds, juce::Random& random);

    /** Moves and ages every mote; the dead ones go. */
    void advance (float seconds);

    void clear();
    bool isEmpty() const { return motes.empty(); }
    int  count() const   { return static_cast<int> (motes.size()); }
    const std::vector<DustMote>& all() const { return motes; }

    /** Where and how big to draw a mote now, in layer pixels. */
    DustSprite spriteOf (const DustMote& mote) const;

    /** Opacity over a mote's life (progress 0..1): fades in, holds, fades out. */
    static float fade (float progress);

    /** Size growth over a mote's life (progress 0..1): ease-out from its start size to its end size. */
    static float growth (float progress);

private:
    void emit (float keyCentreX, float keyWidth, float push, juce::Random& random);
    void makeRoom (int needed);

    std::vector<DustMote> motes;
    float pixelsPerUnit {};   ///< layer width / 1200
    float layerHeight {};     ///< board units
    float time {};            ///< seconds the field has run: the flow's phase
};
