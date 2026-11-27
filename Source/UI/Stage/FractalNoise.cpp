#include "FractalNoise.h"
#include <algorithm>
#include <cmath>

namespace
{
    // SVG 1.1, filters.html#feTurbulenceElement: the reference implementation's constants.
    constexpr long randomModulus    = 2147483647;   // RAND_m
    constexpr long randomMultiplier = 16807;        // RAND_a
    constexpr long randomQuotient   = 127773;       // RAND_q = m / a
    constexpr long randomRemainder  = 2836;         // RAND_r = m % a
    constexpr long firstSeed        = 1;
    constexpr int  blockMask        = 0xff;         // BM
    constexpr double perlinOffset   = 0x1000;       // PerlinN
    constexpr double one            = 1.0;
    constexpr double sCurveA        = 3.0;          // t² (3 − 2t)
    constexpr double sCurveB        = 2.0;
    constexpr double octaveStep     = 2.0;
    constexpr double half           = 0.5;
    constexpr float  lowest         = 0.0f;
    constexpr float  highest        = 1.0f;
    constexpr int    xAxis          = 0;
    constexpr int    yAxis          = 1;
    constexpr int    next           = 1;

    long setupSeed (long seed)
    {
        if (seed <= long {})
            seed = -(seed % (randomModulus - next)) + firstSeed;
        if (seed > randomModulus - next)
            seed = randomModulus - next;
        return seed;
    }

    long nextRandom (long seed)
    {
        auto result = randomMultiplier * (seed % randomQuotient) - randomRemainder * (seed / randomQuotient);
        if (result <= long {})
            result += randomModulus;
        return result;
    }

    double sCurve (double along) { return along * along * (sCurveA - sCurveB * along); }
    double lerp (double along, double from, double to) { return from + along * (to - from); }
}

FractalNoise::FractalNoise (int seed)
{
    auto state = setupSeed (seed);
    for (auto& channel : gradients)
    {
        for (int index {}; index < blockSize; ++index)
        {
            lattice[(size_t) index] = index;
            auto& gradient = channel[(size_t) index];
            for (auto& axis : gradient)
            {
                state = nextRandom (state);
                axis = static_cast<double> ((state % (blockSize + blockSize)) - blockSize) / blockSize;
            }
            const auto length = std::sqrt (gradient[xAxis] * gradient[xAxis] + gradient[yAxis] * gradient[yAxis]);
            gradient[xAxis] /= length;
            gradient[yAxis] /= length;
        }
    }

    for (int index = blockSize - next; index > int {}; --index)
    {
        state = nextRandom (state);
        std::swap (lattice[(size_t) index], lattice[(size_t) (state % blockSize)]);
    }

    for (int index {}; index < blockSize + wrapExtra; ++index)
    {
        lattice[(size_t) (blockSize + index)] = lattice[(size_t) index];
        for (auto& channel : gradients)
            channel[(size_t) (blockSize + index)] = channel[(size_t) index];
    }
}

double FractalNoise::noise (int channel, double pointX, double pointY) const
{
    auto shifted = pointX + perlinOffset;
    const auto bx0 = static_cast<int> (shifted) & blockMask;
    const auto bx1 = (bx0 + next) & blockMask;
    const auto rx0 = shifted - static_cast<int> (shifted);
    const auto rx1 = rx0 - one;
    shifted = pointY + perlinOffset;
    const auto by0 = static_cast<int> (shifted) & blockMask;
    const auto by1 = (by0 + next) & blockMask;
    const auto ry0 = shifted - static_cast<int> (shifted);
    const auto ry1 = ry0 - one;

    const auto column0 = lattice[(size_t) bx0];
    const auto column1 = lattice[(size_t) bx1];
    const auto& table = gradients[(size_t) channel];
    const auto& g00 = table[(size_t) lattice[(size_t) (column0 + by0)]];
    const auto& g10 = table[(size_t) lattice[(size_t) (column1 + by0)]];
    const auto& g01 = table[(size_t) lattice[(size_t) (column0 + by1)]];
    const auto& g11 = table[(size_t) lattice[(size_t) (column1 + by1)]];

    const auto sx = sCurve (rx0);
    const auto sy = sCurve (ry0);
    const auto lowerRow = lerp (sx, rx0 * g00[xAxis] + ry0 * g00[yAxis], rx1 * g10[xAxis] + ry0 * g10[yAxis]);
    const auto upperRow = lerp (sx, rx0 * g01[xAxis] + ry1 * g01[yAxis], rx1 * g11[xAxis] + ry1 * g11[yAxis]);
    return lerp (sy, lowerRow, upperRow);
}

float FractalNoise::sample (int channel, double boardX, double boardY, double frequencyX, double frequencyY, int octaves, Type type) const
{
    auto pointX = boardX * frequencyX;
    auto pointY = boardY * frequencyY;
    double sum {}, ratio = one;
    for (int octave {}; octave < octaves; ++octave)
    {
        const auto value = noise (channel, pointX, pointY);
        sum += (type == Type::fractalNoise ? value : std::abs (value)) / ratio;
        pointX *= octaveStep;
        pointY *= octaveStep;
        ratio *= octaveStep;
    }
    const auto result = type == Type::fractalNoise ? (sum + one) * half : sum;
    return std::clamp (static_cast<float> (result), lowest, highest);
}
