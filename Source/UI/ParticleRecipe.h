#pragma once
#include <JuceHeader.h>

/** Particle looks of the note layer (the particle-style setting). */
namespace ParticleStyles
{
    constexpr int sparks = 1;
    constexpr int dust   = 2;
    constexpr int wisps  = 3;   ///< stored as 3, the value Smoke had (Light wisps replaced Smoke, 2026-11-27)

    /** A style read from the settings file: anything but Sparks / Dust / Wisps reads as Dust (the default). */
    constexpr int fromStored (int stored) { return stored >= sparks && stored <= wisps ? stored : dust; }
}

namespace ParticleLife
{
    inline constexpr float full = 1.0f;
}

/** One particle of the note layer: position and velocity in NDC (-1..1, y up), life 1 -> 0, size in px (see
    ParticleRecipe::pointSize), colour with its own alpha. Wisps move as a function of their age from where they
    started, so their trail is computed instead of stored. */
struct NoteParticle
{
    juce::Point<float> pos;
    juce::Point<float> velocity;
    juce::Point<float> origin;   ///< where it started (Wisps)
    float life = ParticleLife::full;
    float lifespan {};   ///< the life it started with
    float size {};
    float age {};        ///< seconds since it started
    float seed {};       ///< its own phase (Wisps' sway, Sparks' flicker)
    float sway {};       ///< Wisps: NDC of x per NDC of y, so the sway is as wide in pixels as it is tall
    juce::Colour colour;
};

/** One drawn point of a particle: the particle itself (index 0) or a point of its trail. */
struct TrailPoint
{
    juce::Point<float> pos;
    float size {};
    float opacity {};   ///< 0: nothing to draw
    float age {};       ///< the particle's age at this point of its path
};

/*  How Sparks and Wisps look and move (spec 2026-11-27-particles-sparks-smoke): pure, so the rules are unit-tested;
    NoteLayer keeps the storage and the GL drawing. Dust is DustField (board `Dust`, 2026-11-27): no particles here. */
namespace ParticleRecipe
{
    /** Particles spawned for one note (0 for Dust). */
    int perNote (int style);

    /** A new particle above a key (keyTop in NDC), in the user's particle colour. `xPerY` is the layer's height over
        its width, so a motion can be as wide in pixels as it is tall. */
    NoteParticle spawn (int style, juce::Point<float> keyTop, float xPerY, juce::Colour colour, juce::Random& random);

    /** Moves and ages a particle by `seconds`. */
    void step (int style, NoteParticle& particle, float seconds);

    /** How opaque the particle is now: its colour's alpha, fading with the life it has left. */
    float opacity (const NoteParticle& particle);

    /** Whether the style is drawn over the falling notes (Sparks, Wisps) or under them (Dust). */
    bool drawnOverNotes (int style);

    /** Whether the style adds light instead of covering what is under it (all three: Dust is the board's 'lighter'). */
    bool additive (int style);

    /** The GL point size in physical px: a size in logical px times the rendering scale. */
    float pointSize (float size, float renderingScale);

    /** Points drawn per particle at most: the particle and its trail. */
    int trailPoints (int style);

    /** Point `index` of the particle (0 = the particle, then back along its trail); opacity 0 past its start. */
    TrailPoint trailPoint (int style, const NoteParticle& particle, int index);
}
