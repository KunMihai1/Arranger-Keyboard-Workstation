#include <JuceHeader.h>
#include "UI/Stage/FractalNoise.h"
#include "UI/Stage/PanelFinish.h"
#include "AppColours.h"

/*  Panel finishes (board SkinsFinish, redesign B4). The noise oracle is Edge's own rendering of feTurbulence with
    the same parameters (canvas getImageData, color-interpolation-filters sRGB): the SVG reference algorithm matches
    it within Skia's float rounding, while a wrong seed or frequency is far off. */

namespace PanelFinishTestValues
{
    const juce::String testName     = "PanelFinish";
    const juce::String testCategory = "Unit";

    const juce::String noiseMatchesEdge = "fractal noise matches Edge's feTurbulence at sample pixels (fractalNoise and turbulence)";
    const juce::String noiseIsSeeded    = "the noise depends on the seed only: same seed same texture, another seed another";
    const juce::String storedIds        = "each finish saves as its text id and reads back; anything else is Stage";
    const juce::String surfaces         = "each finish paints the board's colours; Stage paints nothing (the default panel stays)";
    const juce::String sides            = "the side strips are the board's woods and blacks";
    const juce::String labelsReadable   = "the panel's printed labels stay readable on every finish (dark ink on Champagne)";
    const juce::String contrastMessage  = "lowest label contrast on ";
    const juce::String renderTime       = "render time of each finish's surface at 1920 x 198 (logged)";
    const juce::String renderTimeMessage = "ms to render ";
    const juce::String versus           = " vs ";
    const juce::String unknownId        = "pearl-accordion";   // on the board, not built (owner)
    const juce::String stageId          = "stage";
    const juce::String rossoId          = "rosso-burst";
    const juce::String concertId        = "concert-black";

    struct EdgeSample { int column, row, red, green, blue, alpha; };

    // feTurbulence type="fractalNoise" baseFrequency="0.045 0.012" numOctaves="3" seed="12" (the board's flame)
    constexpr double fractalFrequencyX = 0.045;
    constexpr double fractalFrequencyY = 0.012;
    constexpr int fractalOctaves = 3;
    constexpr int fractalSeed = 12;
    const std::array<EdgeSample, 10> fractalSamples { { { 0, 0, 118, 116, 141, 141 }, { 44, 0, 130, 121, 128, 129 },
        { 88, 0, 124, 124, 129, 125 }, { 33, 7, 155, 108, 135, 153 }, { 77, 7, 155, 143, 83, 123 }, { 22, 14, 130, 130, 112, 112 },
        { 66, 14, 162, 162, 116, 110 }, { 11, 21, 113, 131, 100, 99 }, { 55, 21, 140, 134, 109, 84 }, { 0, 28, 171, 60, 118, 154 } } };

    // feTurbulence type="turbulence" baseFrequency="0.05 0.08" numOctaves="4" seed="44"
    constexpr double turbulenceFrequencyX = 0.05;
    constexpr double turbulenceFrequencyY = 0.08;
    constexpr int turbulenceOctaves = 4;
    constexpr int turbulenceSeed = 44;
    const std::array<EdgeSample, 10> turbulenceSamples { { { 33, 0, 42, 134, 59, 61 }, { 44, 7, 46, 171, 87, 94 },
        { 11, 14, 136, 152, 187, 131 }, { 66, 14, 92, 141, 62, 78 }, { 44, 21, 97, 130, 78, 171 }, { 88, 21, 75, 58, 108, 106 },
        { 55, 28, 61, 101, 56, 96 }, { 22, 35, 119, 75, 47, 71 }, { 88, 35, 73, 77, 65, 63 }, { 33, 42, 151, 111, 31, 83 } } };

    // Premultiplied 0..255 differences seen against Edge: fractal 7, turbulence 19 (Skia's float rounding).
    constexpr int fractalTolerance    = 10;
    constexpr int turbulenceTolerance = 24;
    constexpr int wrongSeedMinimum    = 40;   // another seed is at least this far somewhere
    constexpr int otherSeed           = 13;

    constexpr float pixelCentre = 0.5f;
    constexpr float fullScale   = 255.0f;
    constexpr int   channels    = 4;
    constexpr int   alphaChannel = 3;

    constexpr int   boardWidth  = 1200;
    constexpr int   panelHeight = 124;
    constexpr float boardScale  = 1.0f;
    constexpr int   sideWidth   = 16;
    constexpr int   centreX     = 600;
    constexpr int   middleY     = 50;
    constexpr int   edgeX       = 2;
    constexpr int   pinstripeY  = 4;
    constexpr int   chromeY     = 122;
    constexpr int   slipHeight  = 12;

    // Where the panel prints labels (board StageRevised, 1200 x 124): section rows, page buttons, Effects, Quick FX.
    struct LabelSpot { int column, row; };
    const std::array<LabelSpot, 11> labelSpots { { { 40, 30 }, { 120, 88 }, { 230, 108 }, { 285, 30 }, { 240, 88 }, { 320, 40 },
                                                    { 320, 106 }, { 880, 40 }, { 960, 40 }, { 1080, 32 }, { 1130, 108 } } };
    constexpr float minimumContrast = 3.0f;   // the labels are small bold caps; 3:1 is the floor for large / bold text

    constexpr int   timedWidth  = 1920;
    constexpr int   timedHeight = 198;
    constexpr float timedScale  = 1.6f;
}

using namespace PanelFinishTestValues;

class PanelFinishTest : public juce::UnitTest
{
public:
    PanelFinishTest() : juce::UnitTest (PanelFinishTestValues::testName, PanelFinishTestValues::testCategory) {}

    void runTest() override
    {
        beginTest (noiseMatchesEdge);
        {
            expectLessOrEqual (worstDifference (FractalNoise (fractalSeed), fractalSamples, fractalFrequencyX, fractalFrequencyY,
                                                fractalOctaves, FractalNoise::Type::fractalNoise), fractalTolerance);
            expectLessOrEqual (worstDifference (FractalNoise (turbulenceSeed), turbulenceSamples, turbulenceFrequencyX, turbulenceFrequencyY,
                                                turbulenceOctaves, FractalNoise::Type::turbulence), turbulenceTolerance);
            // The tolerance tells the algorithm apart: the same samples with another seed are far off.
            expectGreaterThan (worstDifference (FractalNoise (otherSeed), fractalSamples, fractalFrequencyX, fractalFrequencyY,
                                                fractalOctaves, FractalNoise::Type::fractalNoise), wrongSeedMinimum);
        }

        beginTest (noiseIsSeeded);
        {
            const FractalNoise first (fractalSeed), again (fractalSeed);
            const auto sampleOf = [] (const FractalNoise& noise)
                { return noise.sample (int {}, centreX, middleY, fractalFrequencyX, fractalFrequencyY, fractalOctaves, FractalNoise::Type::fractalNoise); };
            expectEquals (sampleOf (first), sampleOf (again));
        }

        beginTest (storedIds);
        {
            for (const auto finish : PanelFinish::all)
                expect (PanelFinish::fromStored (PanelFinish::storedId (finish)) == finish);
            expect (PanelFinish::fromStored (unknownId) == PanelFinish::Id::stage);
            expect (PanelFinish::fromStored ({}) == PanelFinish::Id::stage);
            expectEquals (PanelFinish::storedId (PanelFinish::Id::stage), stageId);
            expectEquals (PanelFinish::storedId (PanelFinish::Id::rossoBurst), rossoId);
            expectEquals (PanelFinish::storedId (PanelFinish::Id::concertBlack), concertId);
        }

        beginTest (surfaces);
        {
            expect (! PanelFinish::renderSurface (PanelFinish::Id::stage, boardWidth, panelHeight, boardScale).isValid());

            const auto rosso = surface (PanelFinish::Id::rossoBurst);
            expect (isRed (rosso.getPixelAt (centreX, middleY)), describe (rosso.getPixelAt (centreX, middleY)));

            const auto champagne = surface (PanelFinish::Id::champagne);
            expect (isLightGold (champagne.getPixelAt (centreX, middleY)), describe (champagne.getPixelAt (centreX, middleY)));

            const auto tobacco = surface (PanelFinish::Id::tobaccoBurst);
            expect (isAmber (tobacco.getPixelAt (centreX, middleY)), describe (tobacco.getPixelAt (centreX, middleY)));
            expect (isDark (tobacco.getPixelAt (edgeX, middleY)), describe (tobacco.getPixelAt (edgeX, middleY)));

            const auto midnight = surface (PanelFinish::Id::midnightFlake);
            expect (isBlue (midnight.getPixelAt (centreX, middleY)), describe (midnight.getPixelAt (centreX, middleY)));
            expect (isChrome (midnight.getPixelAt (centreX, chromeY)), describe (midnight.getPixelAt (centreX, chromeY)));

            // The key slip is the bare surface: no pinstripes on a 12 px strip.
            const auto slip = PanelFinish::renderSurface (PanelFinish::Id::concertBlack, boardWidth, slipHeight, boardScale, PanelFinish::Part::keySlip);
            expect (isDark (slip.getPixelAt (centreX, pinstripeY)), describe (slip.getPixelAt (centreX, pinstripeY)));

            const auto concert = surface (PanelFinish::Id::concertBlack);
            expect (isDark (concert.getPixelAt (centreX, middleY)), describe (concert.getPixelAt (centreX, middleY)));
            expect (isLightGold (concert.getPixelAt (centreX, pinstripeY)), describe (concert.getPixelAt (centreX, pinstripeY)));
        }

        beginTest (sides);
        {
            expect (! PanelFinish::renderSide (PanelFinish::Id::stage, sideWidth, panelHeight, boardScale, false).isValid());
            const auto maple = PanelFinish::renderSide (PanelFinish::Id::rossoBurst, sideWidth, panelHeight, boardScale, false);
            expect (isLightGold (maple.getPixelAt (sideWidth / channels, middleY)), describe (maple.getPixelAt (sideWidth / channels, middleY)));
            for (const auto finish : PanelFinish::all)
                if (finish != PanelFinish::Id::stage)
                {
                    const auto side = PanelFinish::renderSide (finish, sideWidth, panelHeight, boardScale, true);
                    expect (side.isValid() && side.getPixelAt (sideWidth / channels, middleY).getAlpha() > juce::uint8 {});
                }
        }

        beginTest (labelsReadable);
        {
            for (const auto finish : PanelFinish::all)
            {
                if (finish == PanelFinish::Id::stage)
                    continue;
                const auto panel = surface (finish);
                auto lowest = std::numeric_limits<float>::max();
                juce::String perSpot;
                for (const auto& spot : labelSpots)
                {
                    // The panel prints each column in one ink, chosen from the column's average background.
                    const auto columns = PanelFinish::inkColumns ({ boardWidth, panelHeight }, boardScale);
                    const auto column = PanelFinish::inkColumnAt (columns, spot.column);
                    const auto background = averageAround (panel, spot);
                    const auto ink = PanelFinish::inkFor (PanelFinish::averageColour (panel, columns[column]), background, true);
                    const auto spotContrast = contrast (ink, background);
                    perSpot << juce::String (spotContrast, contrastDecimals) << spotSeparator;
                    lowest = juce::jmin (lowest, spotContrast);
                }
                logMessage (contrastMessage + PanelFinish::label (finish) + versus + juce::String (lowest) + spotSeparator + perSpot);
                expectGreaterOrEqual (lowest, minimumContrast, PanelFinish::label (finish));
            }
        }

        beginTest (renderTime);
        {
            for (const auto finish : PanelFinish::all)
            {
                const auto start = juce::Time::getMillisecondCounterHiRes();
                PanelFinish::renderSurface (finish, timedWidth, timedHeight, timedScale);
                logMessage (renderTimeMessage + PanelFinish::label (finish) + versus
                            + juce::String (juce::Time::getMillisecondCounterHiRes() - start));
            }
        }
    }

private:
    static juce::Image surface (PanelFinish::Id finish) { return PanelFinish::renderSurface (finish, boardWidth, panelHeight, boardScale); }

    template <size_t count>
    static int worstDifference (const FractalNoise& noise, const std::array<EdgeSample, count>& samples,
                                double frequencyX, double frequencyY, int octaves, FractalNoise::Type type)
    {
        int worst {};
        for (const auto& sample : samples)
        {
            std::array<float, channels> mine {};
            for (int channel {}; channel < channels; ++channel)
                mine[(size_t) channel] = noise.sample (channel, sample.column + pixelCentre, sample.row + pixelCentre, frequencyX, frequencyY, octaves, type) * fullScale;
            const std::array<int, alphaChannel> edge { sample.red, sample.green, sample.blue };
            for (int channel {}; channel < alphaChannel; ++channel)   // compare as the canvas stores them: premultiplied
            {
                const auto minePremultiplied = mine[(size_t) channel] * mine[alphaChannel] / fullScale;
                const auto edgePremultiplied = static_cast<float> (edge[(size_t) channel] * sample.alpha) / fullScale;
                worst = juce::jmax (worst, juce::roundToInt (std::abs (minePremultiplied - edgePremultiplied)));
            }
            worst = juce::jmax (worst, juce::roundToInt (std::abs (mine[alphaChannel] - static_cast<float> (sample.alpha))));
        }
        return worst;
    }

    /** The background a label sits on: the average over its area (noise like the flake is averaged by the eye). */
    static juce::Colour averageAround (const juce::Image& image, LabelSpot spot)
    {
        float red {}, green {}, blue {};
        int count {};
        for (int row = spot.row - labelHalfHeight; row <= spot.row + labelHalfHeight; ++row)
            for (int column = spot.column - labelHalfWidth; column <= spot.column + labelHalfWidth; ++column)
            {
                const auto pixel = image.getPixelAt (column, row);
                red += pixel.getFloatRed(); green += pixel.getFloatGreen(); blue += pixel.getFloatBlue();
                ++count;
            }
        return juce::Colour::fromFloatRGBA (red / (float) count, green / (float) count, blue / (float) count, fullOpacity);
    }
    static constexpr int labelHalfWidth = 14;
    static constexpr int labelHalfHeight = 4;
    static constexpr float fullOpacity = 1.0f;
    static constexpr int contrastDecimals = 2;
    static inline const juce::String spotSeparator = " ";

    /** WCAG contrast ratio of two colours (relative luminance, sRGB). */
    static float contrast (juce::Colour first, juce::Colour second)
    {
        const auto lighter = juce::jmax (luminance (first), luminance (second));
        const auto darker  = juce::jmin (luminance (first), luminance (second));
        return (lighter + flare) / (darker + flare);
    }
    static float luminance (juce::Colour colour)
    {
        auto linear = [] (juce::uint8 channel)
        {
            const auto value = static_cast<float> (channel) / fullScale;
            return value <= srgbKnee ? value / srgbSlope : std::pow ((value + srgbOffset) / srgbScale, srgbGamma);
        };
        return redWeight * linear (colour.getRed()) + greenWeight * linear (colour.getGreen()) + blueWeight * linear (colour.getBlue());
    }
    static constexpr float flare = 0.05f;
    static constexpr float srgbKnee = 0.04045f;
    static constexpr float srgbSlope = 12.92f;
    static constexpr float srgbOffset = 0.055f;
    static constexpr float srgbScale = 1.055f;
    static constexpr float srgbGamma = 2.4f;
    static constexpr float redWeight = 0.2126f;
    static constexpr float greenWeight = 0.7152f;
    static constexpr float blueWeight = 0.0722f;

    // Colour families of the board's finishes (rough on purpose: they are lit by gradients and noise).
    static bool isRed (juce::Colour colour)       { return colour.getRed() > colour.getGreen() + redMargin && colour.getRed() > redMinimum; }
    static bool isLightGold (juce::Colour colour) { return colour.getRed() > goldRedMinimum && colour.getGreen() > goldGreenMinimum && colour.getRed() >= colour.getBlue(); }
    static bool isAmber (juce::Colour colour)     { return colour.getRed() > amberRedMinimum && colour.getRed() > colour.getGreen() && colour.getGreen() > colour.getBlue(); }
    static bool isBlue (juce::Colour colour)      { return colour.getBlue() > colour.getRed() + blueMargin; }
    static bool isDark (juce::Colour colour)      { return colour.getBrightness() < darkBrightness; }
    static bool isChrome (juce::Colour colour)    { return colour.getBrightness() > chromeBrightness && std::abs ((int) colour.getRed() - (int) colour.getBlue()) < greyMargin; }
    static juce::String describe (juce::Colour colour) { return colour.toDisplayString (true); }

    static constexpr int   redMargin = 80;
    static constexpr int   redMinimum = 150;
    static constexpr int   goldRedMinimum = 150;
    static constexpr int   goldGreenMinimum = 120;
    static constexpr int   amberRedMinimum = 150;
    static constexpr int   blueMargin = 20;
    static constexpr float darkBrightness = 0.3f;
    static constexpr float chromeBrightness = 0.35f;
    static constexpr int   greyMargin = 40;
};

static PanelFinishTest panelFinishTest;
