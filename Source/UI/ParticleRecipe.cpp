#include "ParticleRecipe.h"

namespace
{
    constexpr float centred = 0.5f;   // nextFloat() - centred spreads evenly both ways
    constexpr float halfSize = 0.5f;  // Dust's original point-size rule

    namespace Sparks
    {
        constexpr int   perNote       = 18;
        constexpr float spreadRadians = 0.96f;   // within about 55 degrees of straight up
        constexpr float speedMin      = 0.7f;    // NDC per second
        constexpr float speedSpan     = 0.8f;
        constexpr float gravity       = 2.2f;    // NDC per second squared
        constexpr float lifeMin       = 0.7f;    // seconds
        constexpr float lifeSpan      = 0.4f;
        constexpr float sizeMin       = 3.0f;    // logical px
        constexpr float sizeSpan      = 6.0f;
        constexpr int   trail         = 3;
    }

    namespace Dust   // the original numbers (B3 motion), unchanged
    {
        constexpr int   perNote        = 40;
        constexpr float jitter         = 0.5f;
        constexpr float dragPerTick    = 0.95f;
        constexpr float ticksPerSecond = 60.0f;
        constexpr float fadeRate       = 0.6f;
        constexpr float spreadX        = 0.08f;
        constexpr float spreadY        = 0.02f;
        constexpr float spreadVelocity = 0.2f;
        constexpr float riseSpan       = 0.4f;
        constexpr float riseMin        = 0.1f;
        constexpr float sizeMin        = 3.0f;
        constexpr float sizeSpan       = 15.0f;
        constexpr float alphaMin       = 0.4f;
        constexpr float alphaSpan      = 0.6f;
        constexpr float lifeMin        = 0.8f;
        constexpr float lifeSpan       = 0.4f;
        constexpr int   trail          = 1;
    }

    namespace Smoke
    {
        constexpr int   perNote        = 10;
        constexpr float spreadX        = 0.10f;
        constexpr float driftVelocity  = 0.08f;
        constexpr float riseMin        = 0.15f;  // NDC per second
        constexpr float riseSpan       = 0.20f;
        constexpr float wiggle         = 0.2f;
        constexpr float sizeMin        = 10.0f;  // logical px
        constexpr float sizeSpan       = 6.0f;
        constexpr float growthRate     = 45.0f;  // px per second
        constexpr float lifeMin        = 1.6f;   // seconds
        constexpr float lifeSpan       = 0.8f;
        constexpr float alphaMin       = 0.5f;
        constexpr float alphaSpan      = 0.15f;
        constexpr float lighten        = 0.35f;  // toward a soft grey-white, so it reads as smoke in any colour
        const juce::Colour haze { 0xffe2e4e8 };
        constexpr int   trail          = 1;
    }

    float between (juce::Random& random, float lowest, float span) { return lowest + random.nextFloat() * span; }
    float spread (juce::Random& random, float width) { return (random.nextFloat() - centred) * width; }
}

namespace ParticleRecipe
{
    int perNote (int style)
    {
        switch (style)
        {
            case ParticleStyles::sparks: return Sparks::perNote;
            case ParticleStyles::smoke:  return Smoke::perNote;
            default:                     return Dust::perNote;
        }
    }

    NoteParticle spawn (int style, juce::Point<float> keyTop, juce::Colour colour, juce::Random& random)
    {
        NoteParticle particle;
        if (style == ParticleStyles::sparks)
        {
            const auto angle = spread (random, Sparks::spreadRadians + Sparks::spreadRadians);
            const auto speed = between (random, Sparks::speedMin, Sparks::speedSpan);
            particle.pos      = keyTop;
            particle.velocity = { speed * std::sin (angle), speed * std::cos (angle) };
            particle.size     = between (random, Sparks::sizeMin, Sparks::sizeSpan);
            particle.life     = between (random, Sparks::lifeMin, Sparks::lifeSpan);
            particle.lifespan = particle.life;
            particle.colour   = colour;
        }
        else if (style == ParticleStyles::smoke)
        {
            particle.pos      = keyTop + juce::Point<float> (spread (random, Smoke::spreadX), float {});
            particle.velocity = { spread (random, Smoke::driftVelocity), between (random, Smoke::riseMin, Smoke::riseSpan) };
            particle.size     = between (random, Smoke::sizeMin, Smoke::sizeSpan);
            particle.life     = between (random, Smoke::lifeMin, Smoke::lifeSpan);
            particle.lifespan = particle.life;
            particle.colour   = colour.interpolatedWith (Smoke::haze, Smoke::lighten)
                                      .withAlpha (between (random, Smoke::alphaMin, Smoke::alphaSpan));
        }
        else
        {
            particle.pos      = keyTop + juce::Point<float> (spread (random, Dust::spreadX), spread (random, Dust::spreadY));
            particle.velocity = { spread (random, Dust::spreadVelocity), between (random, Dust::riseMin, Dust::riseSpan) };
            particle.size     = between (random, Dust::sizeMin, Dust::sizeSpan);
            particle.colour   = colour.withAlpha (between (random, Dust::alphaMin, Dust::alphaSpan));
            particle.life     = between (random, Dust::lifeMin, Dust::lifeSpan);
            particle.lifespan = {};   // Dust fades by life itself (its original rule)
        }
        return particle;
    }

    void step (int style, NoteParticle& particle, float seconds, juce::Random& random)
    {
        if (style == ParticleStyles::sparks)
        {
            particle.velocity.y -= Sparks::gravity * seconds;
            particle.pos += particle.velocity * seconds;
            particle.life -= seconds;
        }
        else if (style == ParticleStyles::smoke)
        {
            particle.velocity.x += spread (random, Smoke::wiggle) * seconds;
            particle.size += Smoke::growthRate * seconds;
            particle.pos += particle.velocity * seconds;
            particle.life -= seconds;
        }
        else
        {
            particle.velocity.x += spread (random, Dust::jitter) * seconds;   // Brownian
            particle.velocity.x *= std::pow (Dust::dragPerTick, seconds * Dust::ticksPerSecond);
            particle.pos += particle.velocity * seconds;
            particle.life -= seconds * Dust::fadeRate;
        }
    }

    float opacity (const NoteParticle& particle)
    {
        const auto alpha = particle.colour.getFloatAlpha();
        if (particle.lifespan <= float {})
            return alpha * particle.life;   // Dust's original rule: life starts up to 1.2, so it may pass its alpha
        return juce::jlimit (float {}, alpha, alpha * particle.life / particle.lifespan);
    }

    bool drawnOverNotes (int style) { return style == ParticleStyles::sparks || style == ParticleStyles::smoke; }

    bool additive (int style) { return style == ParticleStyles::sparks; }

    float pointSize (int style, float size, float renderingScale)
    {
        return style == ParticleStyles::dust ? size * halfSize : size * renderingScale;
    }

    int trailPoints (int style)
    {
        switch (style)
        {
            case ParticleStyles::sparks: return Sparks::trail;
            case ParticleStyles::smoke:  return Smoke::trail;
            default:                     return Dust::trail;
        }
    }
}
