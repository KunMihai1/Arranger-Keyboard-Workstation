#pragma once
#include <JuceHeader.h>

/** Particle looks of the note layer (the particle-style setting). */
namespace ParticleStyles
{
    constexpr int sparks = 1;
    constexpr int dust   = 2;
    constexpr int smoke  = 3;

    /** A style read from the settings file: anything but Sparks / Dust / Smoke reads as Dust (the default). */
    constexpr int fromStored (int stored) { return stored >= sparks && stored <= smoke ? stored : dust; }
}

namespace ParticleLife
{
    inline constexpr float full = 1.0f;
}

/** One particle of the note layer: position and velocity in NDC (-1..1, y up), life 1 -> 0, size in px (see
    ParticleRecipe::pointSize), colour with its own alpha. */
struct NoteParticle
{
    juce::Point<float> pos;
    juce::Point<float> velocity;
    float life = ParticleLife::full;
    float lifespan {};   ///< the life it started with (0: Dust, which fades by life itself)
    float size {};
    juce::Colour colour;
};

/*  How each particle style looks and moves (spec 2026-11-27-particles-sparks-smoke): pure, so the rules are
    unit-tested; NoteLayer keeps the storage and the GL drawing. Dust keeps its original numbers. */
namespace ParticleRecipe
{
    /** Particles spawned for one note. */
    int perNote (int style);

    /** A new particle above a key (keyTop in NDC), in the user's particle colour. */
    NoteParticle spawn (int style, juce::Point<float> keyTop, juce::Colour colour, juce::Random& random);

    /** Moves and ages a particle by `seconds`. */
    void step (int style, NoteParticle& particle, float seconds, juce::Random& random);

    /** How opaque the particle is now: its colour's alpha, fading with the life it has left (Dust: alpha × life,
        unclamped, as it always was). */
    float opacity (const NoteParticle& particle);

    /** Whether the style is drawn over the falling notes (Sparks, Smoke) or under them (Dust). */
    bool drawnOverNotes (int style);

    /** Whether the style adds light (Sparks glow) instead of covering what is under it. */
    bool additive (int style);

    /** The GL point size in physical px: Sparks and Smoke sizes are logical px times the rendering scale; Dust keeps
        its original half-size rule. */
    float pointSize (int style, float size, float renderingScale);

    /** Points drawn per particle: Sparks draw a short fading trail behind each spark; the others one point. */
    int trailPoints (int style);
}
