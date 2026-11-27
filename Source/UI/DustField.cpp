#include "DustField.h"

namespace
{
    // Board `Dust` (Dust.dc.html, mode "new"): every number is the board's, in its 1200-wide units.
    namespace Board
    {
        constexpr float width          = 1200.0f;
        constexpr float aboveKeys      = 1.0f;     // emitted this far above the key tops
        constexpr float keySpread      = 0.9f;     // across this share of the key's width
        constexpr float centred        = 0.5f;     // nextFloat() - centred spreads evenly both ways
        constexpr float burstPush      = 1.8f;     // a burst rises faster than the stream
        constexpr float streamPush     = 1.0f;
        constexpr float streamPerSecond = 60.0f;

        constexpr float moteShare      = 0.14f;
        constexpr float sideways       = 26.0f;    // x speed spread (±13)
        constexpr float riseMin        = 14.0f;
        constexpr float riseSpan       = 30.0f;
        constexpr float seedSpan       = 100.0f;

        constexpr float moteLifeMin    = 1.8f;
        constexpr float moteLifeSpan   = 1.4f;
        constexpr float moteStartMin   = 5.0f;
        constexpr float moteStartSpan  = 4.0f;
        constexpr float moteEndMin     = 11.0f;
        constexpr float moteEndSpan    = 8.0f;
        constexpr float moteAlphaMin   = 0.05f;
        constexpr float moteAlphaSpan  = 0.06f;

        constexpr float speckLifeMin   = 2.2f;
        constexpr float speckLifeSpan  = 1.8f;
        constexpr float speckStartMin  = 0.7f;
        constexpr float speckStartSpan = 1.1f;
        constexpr float speckEndMin    = 1.4f;
        constexpr float speckEndSpan   = 1.6f;
        constexpr float speckAlphaMin  = 0.45f;
        constexpr float speckAlphaSpan = 0.45f;
        constexpr float speckRadius    = 2.2f;     // a speck is drawn 2.2 x its size
        constexpr float moteRadius     = 1.0f;

        // The flow field: two waves each way, drifting with time.
        constexpr float damping        = 1.4f;     // velocity kept: e^(-1.4 dt)
        constexpr float flowPullX      = 22.0f;
        constexpr float flowPullY      = 9.0f;
        constexpr float lift           = 16.0f;    // upward pull, strongest when young
        constexpr float waveA          = 0.65f;
        constexpr float waveAy         = 0.011f;
        constexpr float waveAtime      = 0.55f;
        constexpr float waveAseed      = 0.02f;
        constexpr float waveB          = 0.35f;
        constexpr float waveBy         = 0.029f;
        constexpr float waveBtime      = -0.9f;
        constexpr float waveBx         = 0.004f;
        constexpr float waveC          = 0.5f;
        constexpr float waveCx         = 0.009f;
        constexpr float waveCtime      = -0.45f;
        constexpr float waveD          = 0.3f;
        constexpr float waveDx         = 0.023f;
        constexpr float waveDy         = 0.006f;
        constexpr float waveDtime      = 0.8f;

        constexpr float fadeInEnd      = 0.12f;
        constexpr float fadeOutStart   = 0.3f;
    }

    constexpr float whole   = 1.0f;
    constexpr float nothing = 0.0f;
    constexpr float smoothA = 3.0f;   // smoothstep: t² (3 − 2t)
    constexpr float smoothB = 2.0f;

    float between (juce::Random& random, float lowest, float span) { return lowest + random.nextFloat() * span; }

    float smoothStep (float edgeLow, float edgeHigh, float value)
    {
        const auto along = juce::jlimit (nothing, whole, (value - edgeLow) / (edgeHigh - edgeLow));
        return along * along * (smoothA - smoothB * along);
    }
}

DustField::DustField()
{
    motes.reserve (static_cast<size_t> (maxMotes));
}

void DustField::setLayerSize (float widthPixels, float heightPixels)
{
    pixelsPerUnit = widthPixels / Board::width;
    layerHeight = pixelsPerUnit > nothing ? heightPixels / pixelsPerUnit : nothing;
}

void DustField::burst (float keyCentreX, float keyWidth, juce::Random& random)
{
    makeRoom (burstCount);
    for (int index {}; index < burstCount; ++index)
        emit (keyCentreX, keyWidth, Board::burstPush, random);
}

void DustField::emitHeld (float keyCentreX, float keyWidth, float seconds, juce::Random& random)
{
    const auto expected = Board::streamPerSecond * seconds;
    auto count = static_cast<int> (std::floor (expected));
    if (random.nextFloat() < expected - std::floor (expected))
        ++count;

    makeRoom (count);
    for (int index {}; index < count; ++index)
        emit (keyCentreX, keyWidth, Board::streamPush, random);
}

void DustField::emit (float keyCentreX, float keyWidth, float push, juce::Random& random)
{
    if (pixelsPerUnit <= nothing)
        return;

    DustMote mote;
    mote.isMote    = random.nextFloat() < Board::moteShare;
    mote.pos       = { keyCentreX / pixelsPerUnit + (random.nextFloat() - Board::centred) * keyWidth / pixelsPerUnit * Board::keySpread,
                       layerHeight - Board::aboveKeys };
    mote.velocity  = { (random.nextFloat() - Board::centred) * Board::sideways, -between (random, Board::riseMin, Board::riseSpan) * push };
    mote.life      = mote.isMote ? between (random, Board::moteLifeMin, Board::moteLifeSpan) : between (random, Board::speckLifeMin, Board::speckLifeSpan);
    mote.sizeStart = mote.isMote ? between (random, Board::moteStartMin, Board::moteStartSpan) : between (random, Board::speckStartMin, Board::speckStartSpan);
    mote.sizeEnd   = mote.isMote ? between (random, Board::moteEndMin, Board::moteEndSpan) : between (random, Board::speckEndMin, Board::speckEndSpan);
    mote.alpha     = mote.isMote ? between (random, Board::moteAlphaMin, Board::moteAlphaSpan) : between (random, Board::speckAlphaMin, Board::speckAlphaSpan);
    mote.seed      = random.nextFloat() * Board::seedSpan;
    motes.push_back (mote);
}

void DustField::makeRoom (int needed)
{
    // The oldest are at the front (motes are appended and removed in order).
    const auto excess = count() + needed - maxMotes;
    if (excess > int {})
        motes.erase (motes.begin(), motes.begin() + std::min (excess, count()));
}

void DustField::advance (float seconds)
{
    time += seconds;
    const auto kept = std::exp (-Board::damping * seconds);

    for (auto& mote : motes)
    {
        mote.age += seconds;
        const auto progress = mote.age / mote.life;
        const auto flowX = std::sin (mote.pos.y * Board::waveAy + time * Board::waveAtime + mote.seed * Board::waveAseed) * Board::waveA
                         + std::sin (mote.pos.y * Board::waveBy + time * Board::waveBtime + mote.pos.x * Board::waveBx) * Board::waveB;
        const auto flowY = std::cos (mote.pos.x * Board::waveCx + time * Board::waveCtime) * Board::waveC
                         + std::cos (mote.pos.x * Board::waveDx + mote.pos.y * Board::waveDy + time * Board::waveDtime) * Board::waveD;
        mote.velocity.x = mote.velocity.x * kept + flowX * Board::flowPullX * seconds;
        mote.velocity.y = mote.velocity.y * kept + (flowY * Board::flowPullY - Board::lift * (whole - progress)) * seconds;
        mote.pos += mote.velocity * seconds;
    }

    motes.erase (std::remove_if (motes.begin(), motes.end(), [] (const DustMote& mote) { return mote.age >= mote.life; }), motes.end());
}

void DustField::clear()
{
    motes.clear();
}

DustSprite DustField::spriteOf (const DustMote& mote) const
{
    const auto progress = mote.life > nothing ? mote.age / mote.life : whole;
    const auto size = mote.sizeStart + (mote.sizeEnd - mote.sizeStart) * growth (progress);
    const auto radius = size * (mote.isMote ? Board::moteRadius : Board::speckRadius);
    return { mote.pos * pixelsPerUnit, radius * pixelsPerUnit, mote.alpha * fade (progress) };
}

float DustField::fade (float progress)
{
    return smoothStep (nothing, Board::fadeInEnd, progress) * (whole - smoothStep (Board::fadeOutStart, whole, progress));
}

float DustField::growth (float progress)
{
    const auto left = whole - juce::jlimit (nothing, whole, progress);
    return whole - left * left;
}
