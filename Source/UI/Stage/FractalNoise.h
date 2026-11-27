#pragma once

#include <array>

/*  The SVG 1.1 feTurbulence noise (the spec's reference algorithm): a Perlin lattice and gradients made from the seed,
    summed over octaves as "fractalNoise" or "turbulence". The finish boards (SkinsFinish) describe their wood grain,
    flame, brushed metal and flake with feTurbulence; this makes the same textures in C++. Pure. */
class FractalNoise
{
public:
    enum class Type { fractalNoise, turbulence };

    explicit FractalNoise (int seed);

    /** One channel (0 = red .. 3 = alpha) at a point in board units, 0..1 as feTurbulence outputs it
        (fractalNoise: (sum + 1) / 2, turbulence: sum of |noise|; clamped). */
    float sample (int channel, double boardX, double boardY, double frequencyX, double frequencyY, int octaves, Type type) const;

private:
    static constexpr int blockSize = 0x100;
    static constexpr int wrapExtra = 2;   // the reference keeps two extra entries past the doubled block
    static constexpr int tableSize = blockSize + blockSize + wrapExtra;
    static constexpr int channels  = 4;
    static constexpr int axes      = 2;

    double noise (int channel, double pointX, double pointY) const;

    std::array<int, tableSize> lattice {};
    std::array<std::array<std::array<double, axes>, tableSize>, channels> gradients {};
};
