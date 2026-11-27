#include <JuceHeader.h>
#include "UI/ParticleRecipe.h"

/*  Owner (2026-11-27): "the sparks can't even be seen ... smoke is like in the background transparent". First pass
    made sparks visible; on the browser preview the owner then said sparks were "way too boom" (picked the calmer
    embers) and chose Light wisps to replace Smoke ("something that flows a bit and looks smoother").
    Dust stays as it was. */

namespace ParticleRecipeTestValues
{
    const juce::String testName     = "ParticleRecipe";
    const juce::String testCategory = "Unit";

    const juce::String sparksCalm     = "sparks: a calm handful floating up near vertical, big enough to see, living over a second";
    const juce::String sparksArc      = "sparks rise, then gravity brings them down";
    const juce::String sparksTrail    = "a spark's trail runs back along its motion, smaller and fainter";
    const juce::String wispsFlow      = "wisps rise and sway from side to side";
    const juce::String wispsTrailPath = "a wisp's trail follows the path the wisp actually took";
    const juce::String wispsTrailKey  = "a young wisp's trail stops at the key";
    const juce::String wispsLook      = "wisps live long, glow over the notes and draw a long trail";
    const juce::String drawOrder      = "sparks and wisps are drawn over the falling notes; dust under them";
    const juce::String dustUnchanged  = "dust keeps its numbers: 40 per note, half-size points, not scaled, no aspect";
    const juce::String dustOpacity    = "dust opacity is its alpha times its life, also while life is above full (as before)";

    constexpr int   seed              = 2026;
    constexpr int   samples           = 200;
    constexpr int   maxSparks         = 14;
    constexpr float maxSparkTilt      = 0.5f;     // radians from straight up, in screen pixels
    constexpr float minSparkSize      = 6.0f;     // logical px (the point sprite's diameter)
    constexpr float minSparkLife      = 1.0f;     // seconds
    constexpr float minSparkRise      = 0.2f;     // NDC above the key
    constexpr float maxSparkRise      = 1.0f;     // stays inside the note area (2 NDC tall)
    constexpr float xPerY             = 0.3f;     // a wide layer: one NDC of x is 1 / 0.3 NDC of y in pixels
    constexpr float framesPerSecond   = 60.0f;
    constexpr float frame             = ParticleLife::full / framesPerSecond;
    constexpr int   oneSecondOfFrames = 60;
    constexpr int   twoSecondsOfFrames = 120;
    constexpr int   firstTrailPoint   = 0;
    constexpr int   secondTrailPoint  = 1;
    constexpr int   laterTrailPoint   = 20;
    constexpr float wispAge           = 0.9f;     // seconds
    constexpr float youngWispAge      = 0.05f;
    constexpr float minWispRise       = 0.4f;     // NDC after a second
    constexpr float minWispSway       = 0.003f;   // NDC of x
    constexpr float minWispLife       = 1.6f;
    constexpr int   minWispTrail      = 60;
    constexpr int   singlePoint       = 1;
    constexpr int   dustPerNote       = 40;
    constexpr float dustSize          = 10.0f;
    constexpr float dustPointSize     = 5.0f;
    constexpr float windowScale       = 1.5f;
    constexpr float squareLayer       = 1.0f;
    constexpr float noTime            = 0.0f;
    constexpr float sizeTolerance     = 0.001f;
    constexpr float pathTolerance     = 0.0001f;
    constexpr float dustAlpha         = 0.5f;
    constexpr float dustYoungLife     = 1.2f;     // Dust starts with up to 1.2 life
    constexpr float dustYoungOpacity  = 0.6f;
    constexpr float alphaTolerance    = 0.005f;   // a colour keeps its alpha in 8 bits (0.5 is stored as 128 / 255)
    const juce::Point<float> keyTop { 0.0f, -0.95f };
    const juce::Colour particleColour { 0xfff0b43c };
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

        beginTest (sparksCalm);
        {
            expect (perNote (ParticleStyles::sparks) <= maxSparks);
            for (int sample {}; sample < samples; ++sample)
            {
                const auto spark = spawn (ParticleStyles::sparks, keyTop, xPerY, particleColour, random);
                const auto tilt = std::atan2 (std::abs (spark.velocity.x) / xPerY, spark.velocity.y);
                expect (tilt <= maxSparkTilt);
                expect (spark.size >= minSparkSize);
                expect (spark.life >= minSparkLife);
                const auto peak = highestPoint (spark, random) - keyTop.y;
                expect (peak >= minSparkRise && peak <= maxSparkRise);
            }
            expect (additive (ParticleStyles::sparks));
        }

        beginTest (sparksArc);
        {
            auto spark = spawn (ParticleStyles::sparks, keyTop, xPerY, particleColour, random);
            expect (spark.velocity.y > noTime);
            for (int frameIndex {}; frameIndex < twoSecondsOfFrames; ++frameIndex)
                step (ParticleStyles::sparks, spark, frame, random);
            expect (spark.velocity.y < noTime);
        }

        beginTest (sparksTrail);
        {
            auto spark = spawn (ParticleStyles::sparks, keyTop, xPerY, particleColour, random);
            step (ParticleStyles::sparks, spark, frame, random);
            const auto head = trailPoint (ParticleStyles::sparks, spark, firstTrailPoint);
            const auto behind = trailPoint (ParticleStyles::sparks, spark, secondTrailPoint);
            expect (head.pos == spark.pos);
            expect (head.opacity > behind.opacity && behind.opacity > noTime);
            expect (behind.size < head.size);
            expect (behind.pos.y < head.pos.y);   // it is rising, so its past is below
        }

        beginTest (wispsFlow);
        {
            for (int sample {}; sample < samples; ++sample)
            {
                auto wisp = spawn (ParticleStyles::wisps, keyTop, xPerY, particleColour, random);
                const auto start = wisp.pos;
                float widest {};
                for (int frameIndex {}; frameIndex < oneSecondOfFrames; ++frameIndex)
                {
                    step (ParticleStyles::wisps, wisp, frame, random);
                    widest = juce::jmax (widest, std::abs (wisp.pos.x - start.x - wisp.velocity.x * wisp.age));
                }
                expect (wisp.pos.y - start.y >= minWispRise);
                expect (widest >= minWispSway);
            }
        }

        beginTest (wispsTrailPath);
        {
            const auto born = spawn (ParticleStyles::wisps, keyTop, xPerY, particleColour, random);
            auto wisp = born;
            for (float age {}; age < wispAge; age += frame)
                step (ParticleStyles::wisps, wisp, frame, random);
            const auto back = trailPoint (ParticleStyles::wisps, wisp, laterTrailPoint);
            auto earlier = born;
            step (ParticleStyles::wisps, earlier, back.age, random);
            expectWithinAbsoluteError (back.pos.x, earlier.pos.x, pathTolerance);
            expectWithinAbsoluteError (back.pos.y, earlier.pos.y, pathTolerance);
            expect (back.opacity > noTime);
            expect (trailPoint (ParticleStyles::wisps, wisp, firstTrailPoint).pos == wisp.pos);
        }

        beginTest (wispsTrailKey);
        {
            auto wisp = spawn (ParticleStyles::wisps, keyTop, xPerY, particleColour, random);
            step (ParticleStyles::wisps, wisp, youngWispAge, random);
            expectEquals (trailPoint (ParticleStyles::wisps, wisp, trailPoints (ParticleStyles::wisps) - singlePoint).opacity, noTime);
        }

        beginTest (wispsLook);
        {
            const auto wisp = spawn (ParticleStyles::wisps, keyTop, xPerY, particleColour, random);
            expect (wisp.lifespan >= minWispLife);
            expect (additive (ParticleStyles::wisps));
            expect (trailPoints (ParticleStyles::wisps) >= minWispTrail);
            expectEquals (ParticleStyles::fromStored (ParticleStyles::wisps), ParticleStyles::wisps);
        }

        beginTest (drawOrder);
        {
            expect (drawnOverNotes (ParticleStyles::sparks));
            expect (drawnOverNotes (ParticleStyles::wisps));
            expect (! drawnOverNotes (ParticleStyles::dust));
        }

        beginTest (dustUnchanged);
        {
            expectEquals (perNote (ParticleStyles::dust), dustPerNote);
            expectEquals (pointSize (ParticleStyles::dust, dustSize, windowScale), dustPointSize);
            expectEquals (trailPoints (ParticleStyles::dust), singlePoint);
            expect (! additive (ParticleStyles::dust));
            juce::Random wide (seed), square (seed);
            const auto onWide = spawn (ParticleStyles::dust, keyTop, xPerY, particleColour, wide);
            const auto onSquare = spawn (ParticleStyles::dust, keyTop, squareLayer, particleColour, square);
            expect (onWide.pos == onSquare.pos && onWide.velocity == onSquare.velocity);
            const auto mote = trailPoint (ParticleStyles::dust, onWide, firstTrailPoint);
            expect (mote.pos == onWide.pos);
            expectWithinAbsoluteError (mote.size, onWide.size, sizeTolerance);
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
    static float highestPoint (NoteParticle particle, juce::Random& random)
    {
        auto highest = particle.pos.y;
        for (int frameIndex {}; particle.life > ParticleRecipeTestValues::noTime && frameIndex < ParticleRecipeTestValues::samples; ++frameIndex)
        {
            ParticleRecipe::step (ParticleStyles::sparks, particle, ParticleRecipeTestValues::frame, random);
            highest = juce::jmax (highest, particle.pos.y);
        }
        return highest;
    }
};
static ParticleRecipeTest particleRecipeTest;
