#include "ParticleRecipe.h"

namespace
{
    constexpr float centred  = 0.5f;   // nextFloat() - centred spreads evenly both ways
    constexpr float halfSize = 0.5f;   // Dust's original point-size rule
    constexpr float fullTurn = juce::MathConstants<float>::twoPi;
    constexpr float whole    = 1.0f;
    constexpr int   head     = 0;      // trail index of the particle itself

    // Sizes are logical px (the point sprite's diameter); motion is NDC of y per second, and x is scaled by the
    // layer's height / width so a spark or wisp moves as far sideways in pixels as the numbers say.
    namespace Sparks   // calm embers (owner: the first version was "way too boom")
    {
        constexpr int   perNote        = 12;
        constexpr float spreadRadians  = 0.45f;   // within about 26 degrees of straight up
        constexpr float sideways       = 0.6f;    // the sideways part of the launch, damped
        constexpr float speedMin       = 0.84f;
        constexpr float speedSpan      = 0.6f;
        constexpr float gravity        = 1.1f;    // NDC per second squared
        constexpr float dragX          = 0.6f;    // velocity kept per second
        constexpr float dragY          = 0.85f;
        constexpr float lifeMin        = 1.1f;    // seconds
        constexpr float lifeSpan       = 0.7f;
        constexpr float sizeMin        = 7.0f;
        constexpr float sizeSpan       = 6.0f;
        constexpr int   trail          = 5;
        constexpr float trailStep      = 0.016f;  // seconds between trail points
        constexpr float trailShrink    = 0.12f;
        constexpr float trailGlow      = 0.35f;   // a trail point's opacity next to the spark's
        constexpr float trailFadeSpan  = 6.0f;
        constexpr float flickerBase    = 0.8f;
        constexpr float flickerDepth   = 0.2f;
        constexpr float flickerRate    = 25.0f;   // radians per second
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

    namespace Wisps   // thin glowing trails that curl up from the key (replaced Smoke)
    {
        constexpr int   perNote        = 5;
        constexpr float spreadX        = 0.023f;  // NDC of x: about a key's width
        constexpr float driftSpan      = 0.08f;
        constexpr float riseMin        = 0.76f;
        constexpr float riseSpan       = 0.4f;
        constexpr float riseKept       = 0.7f;    // rising speed kept per second
        constexpr float swayWide       = 0.11f;   // two sways, NDC of y before the aspect
        constexpr float swayWideRate   = 3.2f;    // radians per second
        constexpr float swayNarrow     = 0.066f;
        constexpr float swayNarrowRate = 2.0f;
        constexpr float secondPhase    = 2.0f;
        constexpr float lifeMin        = 1.6f;
        constexpr float lifeSpan       = 0.6f;
        constexpr float size           = 6.0f;
        constexpr float alpha          = 0.7f;
        constexpr float lighten        = 0.35f;   // toward white, so the trail reads as light in any colour
        constexpr int   trail          = 110;
        constexpr float trailStep      = 0.006f;  // seconds: dense enough that the points read as one line
        constexpr float thinnest       = 0.3f;    // the trail's tail is this much of the head's size
        constexpr float overlap        = 0.6f;    // neighbouring points overlap, so each adds less light
    }

    float between (juce::Random& random, float lowest, float span) { return lowest + random.nextFloat() * span; }
    float spread (juce::Random& random, float width) { return (random.nextFloat() - centred) * width; }

    /** Where a wisp is at `age`: it rises, slowing, and sways on two waves — closed form, so the trail is its past. */
    juce::Point<float> wispAt (const NoteParticle& wisp, float age)
    {
        const auto slowing = -std::log (Wisps::riseKept);
        const auto rise = wisp.velocity.y * (whole - std::exp (-slowing * age)) / slowing;
        const auto secondSeed = wisp.seed * Wisps::secondPhase;
        const auto sway = Wisps::swayWide * (std::sin (Wisps::swayWideRate * age + wisp.seed) - std::sin (wisp.seed))
                        + Wisps::swayNarrow * (std::sin (Wisps::swayNarrowRate * age + secondSeed) - std::sin (secondSeed));
        return { wisp.origin.x + wisp.velocity.x * age + wisp.sway * sway, wisp.origin.y + rise };
    }
}

namespace ParticleRecipe
{
    int perNote (int style)
    {
        switch (style)
        {
            case ParticleStyles::sparks: return Sparks::perNote;
            case ParticleStyles::wisps:  return Wisps::perNote;
            default:                     return Dust::perNote;
        }
    }

    NoteParticle spawn (int style, juce::Point<float> keyTop, float xPerY, juce::Colour colour, juce::Random& random)
    {
        NoteParticle particle;
        if (style == ParticleStyles::sparks)
        {
            const auto angle = spread (random, Sparks::spreadRadians + Sparks::spreadRadians);
            const auto speed = between (random, Sparks::speedMin, Sparks::speedSpan);
            particle.pos      = keyTop;
            particle.origin   = keyTop;
            particle.velocity = { speed * std::sin (angle) * Sparks::sideways * xPerY, speed * std::cos (angle) };
            particle.size     = between (random, Sparks::sizeMin, Sparks::sizeSpan);
            particle.life     = between (random, Sparks::lifeMin, Sparks::lifeSpan);
            particle.lifespan = particle.life;
            particle.seed     = random.nextFloat() * fullTurn;
            particle.colour   = colour;
        }
        else if (style == ParticleStyles::wisps)
        {
            particle.origin   = keyTop + juce::Point<float> (spread (random, Wisps::spreadX), float {});
            particle.pos      = particle.origin;
            particle.velocity = { spread (random, Wisps::driftSpan) * xPerY, between (random, Wisps::riseMin, Wisps::riseSpan) };
            particle.size     = Wisps::size;
            particle.life     = between (random, Wisps::lifeMin, Wisps::lifeSpan);
            particle.lifespan = particle.life;
            particle.seed     = random.nextFloat() * fullTurn;
            particle.sway     = xPerY;
            particle.colour   = colour.interpolatedWith (juce::Colours::white, Wisps::lighten).withAlpha (Wisps::alpha);
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
        particle.age += seconds;
        if (style == ParticleStyles::sparks)
        {
            particle.velocity.y -= Sparks::gravity * seconds;
            particle.velocity.x *= std::pow (Sparks::dragX, seconds);
            particle.velocity.y *= std::pow (Sparks::dragY, seconds);
            particle.pos += particle.velocity * seconds;
            particle.life -= seconds;
        }
        else if (style == ParticleStyles::wisps)
        {
            particle.pos = wispAt (particle, particle.age);
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

    bool drawnOverNotes (int style) { return style == ParticleStyles::sparks || style == ParticleStyles::wisps; }

    bool additive (int style) { return style == ParticleStyles::sparks || style == ParticleStyles::wisps; }

    float pointSize (int style, float size, float renderingScale)
    {
        return style == ParticleStyles::dust ? size * halfSize : size * renderingScale;
    }

    int trailPoints (int style)
    {
        switch (style)
        {
            case ParticleStyles::sparks: return Sparks::trail;
            case ParticleStyles::wisps:  return Wisps::trail;
            default:                     return Dust::trail;
        }
    }

    TrailPoint trailPoint (int style, const NoteParticle& particle, int index)
    {
        if (index < head || index >= trailPoints (style))
            return {};

        const auto back = static_cast<float> (index);
        if (style == ParticleStyles::sparks)
        {
            const auto flicker = Sparks::flickerBase + Sparks::flickerDepth * std::sin (particle.seed + particle.age * Sparks::flickerRate);
            const auto glow = index == head ? whole : Sparks::trailGlow * (whole - back / Sparks::trailFadeSpan);
            return { particle.pos - particle.velocity * (back * Sparks::trailStep), particle.size * (whole - back * Sparks::trailShrink),
                     opacity (particle) * flicker * glow, particle.age - back * Sparks::trailStep };
        }
        if (style == ParticleStyles::wisps)
        {
            const auto age = particle.age - back * Wisps::trailStep;
            if (age < float {})
                return {};   // the trail starts at the key
            const auto along = whole - back / static_cast<float> (Wisps::trail);
            return { index == head ? particle.pos : wispAt (particle, age), particle.size * (Wisps::thinnest + (whole - Wisps::thinnest) * along),
                     opacity (particle) * along * Wisps::overlap, age };
        }
        return { particle.pos, particle.size, opacity (particle), particle.age };
    }
}
