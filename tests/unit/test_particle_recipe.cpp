#include <JuceHeader.h>
#include "UI/ParticleRecipe.h"

/*  Owner (2026-11-27): "the sparks can't even be seen ... smoke is like in the background transparent". Picked:
    glowing sparks (bigger, white-hot core, trail, arc and fall) and visible smoke wisps drawn over the notes.
    Dust stays as it was. */

namespace ParticleRecipeTestValues
{
    const juce::String testName     = "ParticleRecipe";
    const juce::String testCategory = "Unit";

    const juce::String sparksVisible  = "sparks: enough of them, big enough to see at any scale, and they live long enough";
    const juce::String sparksArc      = "sparks burst upwards and gravity brings them down";
    const juce::String smokeVisible   = "smoke: starts at least half opaque, grows and thins out over seconds";
    const juce::String drawOrder      = "sparks and smoke are drawn over the falling notes; dust under them";
    const juce::String dustUnchanged  = "dust keeps its numbers: 40 per note, half-size points, not scaled";
    const juce::String dustOpacity    = "dust opacity is its alpha times its life, also while life is above full (as before)";

    constexpr int   seed           = 2026;
    constexpr int   samples        = 200;
    constexpr int   minSparks      = 16;
    constexpr float minSparkSize   = 3.0f;      // logical px
    constexpr float minSparkLife   = 0.7f;      // seconds at the fade rate of 1 life per second
    constexpr float windowScale    = 1.5f;
    constexpr float framesPerSecond = 60.0f;
    constexpr float frame          = ParticleLife::full / framesPerSecond;
    constexpr int   oneSecondOfFrames = 60;
    constexpr int   halfSecondOfFrames = 30;
    constexpr float minSmokeAlpha  = 0.5f;
    constexpr int   sparkTrail     = 3;
    constexpr int   singlePoint    = 1;
    constexpr int   dustPerNote    = 40;
    constexpr float dustSize       = 10.0f;
    constexpr float dustPointSize  = 5.0f;
    constexpr float noTime         = 0.0f;
    constexpr float sizeTolerance  = 0.001f;
    constexpr float dustAlpha      = 0.5f;
    constexpr float dustYoungLife  = 1.2f;      // Dust starts with up to 1.2 life
    constexpr float dustYoungOpacity = 0.6f;
    constexpr float alphaTolerance = 0.005f;    // a colour keeps its alpha in 8 bits (0.5 is stored as 128 / 255)
    const juce::Point<float> keyTop { 0.0f, -0.5f };
    const juce::Colour particleColour { 0xff6fd3ff };
}

class ParticleRecipeTest : public juce::UnitTest
{
public:
    ParticleRecipeTest() : juce::UnitTest (ParticleRecipeTestValues::testName, ParticleRecipeTestValues::testCategory) {}

    void runTest() override
    {
        using namespace ParticleRecipeTestValues;
        using namespace ParticleRecipe;
        juce::Random random (seed);

        beginTest (sparksVisible);
        {
            expect (perNote (ParticleStyles::sparks) >= minSparks);
            for (int sample {}; sample < samples; ++sample)
            {
                const auto spark = spawn (ParticleStyles::sparks, keyTop, particleColour, random);
                expect (spark.size >= minSparkSize);
                expectWithinAbsoluteError (pointSize (ParticleStyles::sparks, spark.size, windowScale), spark.size * windowScale, sizeTolerance);
                expect (static_cast<float> (countFramesAlive (spark, random)) * frame >= minSparkLife);
            }
            expectEquals (trailPoints (ParticleStyles::sparks), sparkTrail);
            expect (additive (ParticleStyles::sparks));
        }

        beginTest (sparksArc);
        {
            auto spark = spawn (ParticleStyles::sparks, keyTop, particleColour, random);
            expect (spark.velocity.y > noTime);
            for (int frameIndex {}; frameIndex < oneSecondOfFrames; ++frameIndex)
                step (ParticleStyles::sparks, spark, frame, random);
            expect (spark.velocity.y < noTime);
        }

        beginTest (smokeVisible);
        {
            for (int sample {}; sample < samples; ++sample)
            {
                auto puff = spawn (ParticleStyles::smoke, keyTop, particleColour, random);
                expect (puff.colour.getFloatAlpha() >= minSmokeAlpha);
                const auto startSize = puff.size;
                const auto startLife = puff.life;
                for (int frameIndex {}; frameIndex < halfSecondOfFrames; ++frameIndex)
                    step (ParticleStyles::smoke, puff, frame, random);
                expect (puff.size > startSize);
                expect (puff.life < startLife && puff.life > noTime);   // thins out, but lives past half a second
                expect (puff.velocity.y > noTime);
            }
            expect (! additive (ParticleStyles::smoke));
        }

        beginTest (drawOrder);
        {
            expect (drawnOverNotes (ParticleStyles::sparks));
            expect (drawnOverNotes (ParticleStyles::smoke));
            expect (! drawnOverNotes (ParticleStyles::dust));
        }

        beginTest (dustUnchanged);
        {
            expectEquals (perNote (ParticleStyles::dust), dustPerNote);
            expectEquals (pointSize (ParticleStyles::dust, dustSize, windowScale), dustPointSize);
            expectEquals (trailPoints (ParticleStyles::dust), singlePoint);
            expect (! additive (ParticleStyles::dust));
        }

        beginTest (dustOpacity);
        {
            NoteParticle mote;
            mote.colour = particleColour.withAlpha (dustAlpha);
            mote.life = dustYoungLife;
            expectWithinAbsoluteError (opacity (mote), dustYoungOpacity, alphaTolerance);
        }
    }

private:
    static int countFramesAlive (NoteParticle particle, juce::Random& random)
    {
        int frames {};
        while (particle.life > ParticleRecipeTestValues::noTime && frames < ParticleRecipeTestValues::samples)
        {
            ParticleRecipe::step (ParticleStyles::sparks, particle, ParticleRecipeTestValues::frame, random);
            ++frames;
        }
        return frames;
    }
};
static ParticleRecipeTest particleRecipeTest;
