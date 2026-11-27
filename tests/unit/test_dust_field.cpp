#include <JuceHeader.h>
#include "DustField.h"

namespace DustFieldTestValues
{
    const juce::String testName     = "DustField";
    const juce::String testCategory = "Unit";

    const juce::String burstOnPress    = "a key press bursts 26 motes from the key's top edge, across 90 % of its width, rising";
    const juce::String streamWhileHeld = "a held key streams 60 motes a second";
    const juce::String twoKinds        = "about 14 % are big soft motes, the rest fine specks, each with the board's ranges";
    const juce::String fadeAndGrow     = "a mote fades in, holds, fades out, and grows with an ease-out";
    const juce::String smoothFlow      = "motion is a smooth flow: the same field moves the same way, no per-frame random kicks";
    const juce::String risesAndDies    = "motes rise and are gone after their life";
    const juce::String budget          = "at most 3200 motes: the oldest go first";
    const juce::String scalesWithWidth = "at 1.6 x the window every position and size is 1.6 x";
    const juce::String boardFormulas   = "one frame moves a mote exactly by the board's flow, lift and damping; specks draw 2.2 x their size";

    constexpr float boardWidth    = 1200.0f;
    constexpr float boardHeight   = 465.0f;     // the board's note area
    constexpr float ownerScale    = 1.6f;
    constexpr float keyCentre     = 600.0f;
    constexpr float keyWidth      = 23.0f;
    constexpr float spreadShare   = 0.45f;      // either side of the centre
    constexpr float topEdgeAbove  = 1.0f;       // emitted 1 unit above the key tops
    constexpr float positionTolerance = 0.01f;

    constexpr int   framesPerSecond = 60;
    constexpr float oneSecond       = 1.0f;
    constexpr float frame           = oneSecond / framesPerSecond;
    constexpr int   streamMin       = 55;
    constexpr int   streamMax       = 65;

    constexpr int   manyBursts      = 400;
    constexpr float moteShareMin    = 0.12f;
    constexpr float moteShareMax    = 0.16f;

    // Board ranges (inclusive)
    constexpr float moteLifeMin    = 1.8f;
    constexpr float moteLifeMax    = 3.2f;
    constexpr float speckLifeMin   = 2.2f;
    constexpr float speckLifeMax   = 4.0f;
    constexpr float moteStartMin   = 5.0f;
    constexpr float moteStartMax   = 9.0f;
    constexpr float moteEndMin     = 11.0f;
    constexpr float moteEndMax     = 19.0f;
    constexpr float speckStartMin  = 0.7f;
    constexpr float speckStartMax  = 1.8f;
    constexpr float speckEndMin    = 1.4f;
    constexpr float speckEndMax    = 3.0f;
    constexpr float moteAlphaMin   = 0.05f;
    constexpr float moteAlphaMax   = 0.11f;
    constexpr float speckAlphaMin  = 0.45f;
    constexpr float speckAlphaMax  = 0.9f;
    constexpr float sidewaysMax    = 13.0f;
    constexpr float burstPush      = 1.8f;
    constexpr float riseMin        = 14.0f;
    constexpr float riseMax        = 44.0f;
    constexpr float burstRiseMin   = riseMin * burstPush;
    constexpr float burstRiseMax   = riseMax * burstPush;
    constexpr float rangeTolerance = 0.001f;

    constexpr float born           = 0.0f;
    constexpr float fadedIn        = 0.12f;
    constexpr float stillFull      = 0.3f;
    constexpr float halfway        = 0.5f;
    constexpr float dead           = 1.0f;
    constexpr float full           = 1.0f;
    constexpr float none           = 0.0f;
    constexpr float easeOutHalfway = 0.75f;
    constexpr float fadingOut      = 0.65f;   // halfway through the fade-out (0.3 .. 1)
    constexpr float halfFaded      = 0.5f;
    constexpr float curveTolerance = 0.001f;

    // One frame changes a velocity by at most the flow's pull (22 x, 9 + 16 y) times dt plus the damping.
    constexpr float flowPullX = 22.0f;
    constexpr float flowPullY = 25.0f;
    constexpr float damping   = 1.4f;
    constexpr int   framesToWatch = 30;

    constexpr int   oneMoreBurst      = 1;
    constexpr int   burstsToFill      = DustField::maxMotes / DustField::burstCount + oneMoreBurst;
    constexpr float longerThanAnyLife = 4.1f;
    constexpr float aFewFrames        = 0.1f;
    constexpr int   seedA = 7;
}

using namespace DustFieldTestValues;

class DustFieldTest : public juce::UnitTest
{
public:
    DustFieldTest() : juce::UnitTest (DustFieldTestValues::testName, DustFieldTestValues::testCategory) {}

    void runTest() override
    {
        beginTest (burstOnPress);
        {
            DustField field;
            field.setLayerSize (boardWidth, boardHeight);
            juce::Random random (seedA);
            field.burst (keyCentre, keyWidth, random);
            expectEquals (field.count(), DustField::burstCount);
            for (const auto& mote : field.all())
            {
                expectWithinAbsoluteError (mote.pos.y, boardHeight - topEdgeAbove, positionTolerance);
                expectLessOrEqual (std::abs (mote.pos.x - keyCentre), keyWidth * spreadShare + positionTolerance);
                expectLessOrEqual (std::abs (mote.velocity.x), sidewaysMax + rangeTolerance);
                expect (-mote.velocity.y >= burstRiseMin - rangeTolerance && -mote.velocity.y <= burstRiseMax + rangeTolerance);
            }
        }

        beginTest (streamWhileHeld);
        {
            DustField field;
            field.setLayerSize (boardWidth, boardHeight);
            juce::Random random (seedA);
            for (int step {}; step < framesPerSecond; ++step)
                field.emitHeld (keyCentre, keyWidth, frame, random);
            expect (field.count() >= streamMin && field.count() <= streamMax, juce::String (field.count()));
        }

        beginTest (twoKinds);
        {
            DustField field;
            field.setLayerSize (boardWidth, boardHeight);
            juce::Random random (seedA);
            int motes {}, all {}, outOfRange {};
            for (int burst {}; burst < manyBursts; ++burst)
            {
                field.clear();
                field.burst (keyCentre, keyWidth, random);
                for (const auto& mote : field.all())
                {
                    ++all;
                    if (mote.isMote) ++motes;
                    const auto inRange = [] (float value, float lowest, float highest)
                        { return value >= lowest - rangeTolerance && value <= highest + rangeTolerance; };
                    const auto ok = mote.isMote
                        ? inRange (mote.life, moteLifeMin, moteLifeMax) && inRange (mote.sizeStart, moteStartMin, moteStartMax)
                              && inRange (mote.sizeEnd, moteEndMin, moteEndMax) && inRange (mote.alpha, moteAlphaMin, moteAlphaMax)
                        : inRange (mote.life, speckLifeMin, speckLifeMax) && inRange (mote.sizeStart, speckStartMin, speckStartMax)
                              && inRange (mote.sizeEnd, speckEndMin, speckEndMax) && inRange (mote.alpha, speckAlphaMin, speckAlphaMax);
                    if (! ok) ++outOfRange;
                }
            }
            expectEquals (outOfRange, int {});
            const auto share = static_cast<float> (motes) / static_cast<float> (all);
            expect (share >= moteShareMin && share <= moteShareMax, juce::String (share));
        }

        beginTest (fadeAndGrow);
        {
            expectWithinAbsoluteError (DustField::fade (born), none, curveTolerance);
            expectWithinAbsoluteError (DustField::fade (fadedIn), full, curveTolerance);
            expectWithinAbsoluteError (DustField::fade (stillFull), full, curveTolerance);
            expectWithinAbsoluteError (DustField::fade (fadingOut), halfFaded, curveTolerance);
            expectWithinAbsoluteError (DustField::fade (dead), none, curveTolerance);
            expectWithinAbsoluteError (DustField::growth (born), none, curveTolerance);
            expectWithinAbsoluteError (DustField::growth (halfway), easeOutHalfway, curveTolerance);
            expectWithinAbsoluteError (DustField::growth (dead), full, curveTolerance);
        }

        beginTest (smoothFlow);
        {
            DustField first, second;
            first.setLayerSize (boardWidth, boardHeight);
            second.setLayerSize (boardWidth, boardHeight);
            juce::Random randomA (seedA), randomB (seedA);
            first.burst (keyCentre, keyWidth, randomA);
            second.burst (keyCentre, keyWidth, randomB);

            int jumps {}, differences {};
            for (int step {}; step < framesToWatch; ++step)
            {
                const auto before = first.all();
                first.advance (frame);
                second.advance (frame);
                for (size_t index {}; index < before.size(); ++index)
                {
                    const auto& now = first.all()[index];
                    const auto damped = before[index].velocity * std::exp (-damping * frame);
                    if (std::abs (now.velocity.x - damped.x) > flowPullX * frame + rangeTolerance
                        || std::abs (now.velocity.y - damped.y) > flowPullY * frame + rangeTolerance)
                        ++jumps;
                    if (now.pos != second.all()[index].pos)
                        ++differences;
                }
            }
            expectEquals (jumps, int {});
            expectEquals (differences, int {});
        }

        beginTest (risesAndDies);
        {
            DustField field;
            field.setLayerSize (boardWidth, boardHeight);
            juce::Random random (seedA);
            field.burst (keyCentre, keyWidth, random);
            for (int step {}; step < framesPerSecond; ++step)
                field.advance (frame);
            for (const auto& mote : field.all())
                expectLessThan (mote.pos.y, boardHeight - topEdgeAbove);
            field.advance (longerThanAnyLife);
            expect (field.isEmpty());
        }

        beginTest (budget);
        {
            DustField field;
            field.setLayerSize (boardWidth, boardHeight);
            juce::Random random (seedA);
            for (int burst {}; burst < burstsToFill; ++burst)   // bounded: a field that emits nothing must fail, not hang
                field.burst (keyCentre, keyWidth, random);
            field.advance (aFewFrames);   // every mote so far is older than a new one
            field.burst (keyCentre, keyWidth, random);
            expectEquals (field.count(), DustField::maxMotes);
            int newborn {};
            for (const auto& mote : field.all())
                if (mote.age <= none) ++newborn;
            expectEquals (newborn, DustField::burstCount);
        }

        beginTest (boardFormulas);
        {
            // The board's step(), written out here with its own numbers (Dust.dc.html), for the first frame of a burst.
            constexpr float waveA = 0.65f;
            constexpr float waveAy = 0.011f;
            constexpr float waveAtime = 0.55f;
            constexpr float waveAseed = 0.02f;
            constexpr float waveB = 0.35f;
            constexpr float waveBy = 0.029f;
            constexpr float waveBtime = 0.9f;
            constexpr float waveBx = 0.004f;
            constexpr float waveC = 0.5f;
            constexpr float waveCx = 0.009f;
            constexpr float waveCtime = 0.45f;
            constexpr float waveD = 0.3f;
            constexpr float waveDx = 0.023f;
            constexpr float waveDy = 0.006f;
            constexpr float waveDtime = 0.8f;
            constexpr float pullX = 22.0f;
            constexpr float pullY = 9.0f;
            constexpr float lift = 16.0f;
            constexpr float keep = 1.4f;
            constexpr float speckRadius = 2.2f;
            constexpr float formulaTolerance = 0.0001f;

            DustField field;
            field.setLayerSize (boardWidth, boardHeight);
            juce::Random random (seedA);
            field.burst (keyCentre, keyWidth, random);
            const auto born = field.all();
            for (const auto& mote : born)
            {
                const auto radius = field.spriteOf (mote).radius;
                expectWithinAbsoluteError (radius, mote.sizeStart * (mote.isMote ? full : speckRadius), formulaTolerance);
            }

            field.advance (frame);
            const auto& mote = born.front();
            const auto time = frame;
            const auto progress = frame / mote.life;
            const auto flowX = std::sin (mote.pos.y * waveAy + time * waveAtime + mote.seed * waveAseed) * waveA
                             + std::sin (mote.pos.y * waveBy - time * waveBtime + mote.pos.x * waveBx) * waveB;
            const auto flowY = std::cos (mote.pos.x * waveCx - time * waveCtime) * waveC
                             + std::cos (mote.pos.x * waveDx + mote.pos.y * waveDy + time * waveDtime) * waveD;
            const auto velocityX = mote.velocity.x * std::exp (-keep * frame) + flowX * pullX * frame;
            const auto velocityY = mote.velocity.y * std::exp (-keep * frame) + (flowY * pullY - lift * (full - progress)) * frame;
            const auto& moved = field.all().front();
            expectWithinAbsoluteError (moved.velocity.x, velocityX, formulaTolerance);
            expectWithinAbsoluteError (moved.velocity.y, velocityY, formulaTolerance);
            expectWithinAbsoluteError (moved.pos.x, mote.pos.x + velocityX * frame, formulaTolerance);
            expectWithinAbsoluteError (moved.pos.y, mote.pos.y + velocityY * frame, formulaTolerance);
            expectWithinAbsoluteError (field.spriteOf (moved).opacity, moved.alpha * DustField::fade (progress), formulaTolerance);
        }

        beginTest (scalesWithWidth);
        {
            DustField board, scaled;
            board.setLayerSize (boardWidth, boardHeight);
            scaled.setLayerSize (boardWidth * ownerScale, boardHeight * ownerScale);
            juce::Random randomA (seedA), randomB (seedA);
            board.burst (keyCentre, keyWidth, randomA);
            scaled.burst (keyCentre * ownerScale, keyWidth * ownerScale, randomB);
            board.advance (frame);
            scaled.advance (frame);
            for (int index {}; index < board.count(); ++index)
            {
                const auto small = board.spriteOf (board.all()[(size_t) index]);
                const auto large = scaled.spriteOf (scaled.all()[(size_t) index]);
                expectWithinAbsoluteError (large.centre.x, small.centre.x * ownerScale, positionTolerance * ownerScale);
                expectWithinAbsoluteError (large.centre.y, small.centre.y * ownerScale, positionTolerance * ownerScale);
                expectWithinAbsoluteError (large.radius, small.radius * ownerScale, positionTolerance);
                expectWithinAbsoluteError (large.opacity, small.opacity, curveTolerance);
            }
        }
    }
};

static DustFieldTest dustFieldTest;
