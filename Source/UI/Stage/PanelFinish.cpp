#include "PanelFinish.h"
#include "FractalNoise.h"
#include "AppColours.h"
#include "StageMetrics.h"
#include <functional>
#include <vector>

namespace
{
    namespace Stored   // PanelFinish settings values: persisted, never change
    {
        const juce::String stage         = "stage";
        const juce::String rossoBurst    = "rosso-burst";
        const juce::String champagne     = "champagne";
        const juce::String tobaccoBurst  = "tobacco-burst";
        const juce::String midnightFlake = "midnight-flake";
        const juce::String concertBlack  = "concert-black";
    }

    namespace Labels
    {
        const juce::String stage         = "Stage";
        const juce::String rossoBurst    = "Rosso";
        const juce::String champagne     = "Champagne";
        const juce::String tobaccoBurst  = "Tobacco";
        const juce::String midnightFlake = "Midnight";
        const juce::String concertBlack  = "Concert";
    }

    struct Stop { float position; juce::Colour colour; };
    using Stops = std::vector<Stop>;

    // CSS gradient angles: 0deg points up, 90deg right, 180deg down.
    constexpr float toRight  = 90.0f;
    constexpr float toBottom = 180.0f;
    constexpr float glossAngle = 100.0f;   // the sheens sweep slightly down to the right
    constexpr float sideStripBoardWidth = 16.0f;   // agent call: 26 on the board would cover the panel's first LEDs (x = 20)

    // ---- Rosso burst: radial-gradient(ellipse 70% 120% at 50% 40%, ...), then a top shine
    constexpr float rossoEllipseWidth = 0.7f;
    constexpr float rossoEllipseHeight = 1.2f;
    constexpr float rossoCentreX = 0.5f;
    constexpr float rossoCentreY = 0.4f;
    const Stops rossoBurst { { 0.0f, FinishPalette::rossoCentre }, { 0.38f, FinishPalette::rossoInner },
                             { 0.72f, FinishPalette::rossoOuter }, { 1.0f, FinishPalette::rossoEdge } };
    const Stops lacquerShine { { 0.0f, FinishPalette::lacquerShine }, { 0.18f, FinishPalette::lacquerShineSoft },
                               { 0.4f, FinishPalette::clearWhite }, { 0.4f, FinishPalette::clearBlack },
                               { 1.0f, FinishPalette::lacquerShadow } };

    // ---- Champagne: gold gradient, brushed noise, a diagonal sheen
    const Stops champagneBase { { 0.0f, FinishPalette::champagneTop }, { 0.45f, FinishPalette::champagneMiddle },
                                { 1.0f, FinishPalette::champagneBottom } };
    const Stops champagneSheen { { 0.0f, FinishPalette::champagneClear }, { 0.2f, FinishPalette::champagneClear },
                                 { 0.42f, FinishPalette::champagneSheen }, { 0.6f, FinishPalette::champagneClear },
                                 { 1.0f, FinishPalette::champagneClear } };

    // ---- Tobacco burst: amber, flame noise, a dark burst at the edges, a top light
    constexpr float tobaccoEllipseWidth = 0.6f;
    constexpr float tobaccoEllipseHeight = 1.4f;
    constexpr float tobaccoCentreX = 0.5f;
    constexpr float tobaccoCentreY = 0.5f;
    const Stops tobaccoBurst { { 0.0f, FinishPalette::tobaccoClear }, { 0.3f, FinishPalette::tobaccoClear },
                               { 0.58f, FinishPalette::tobaccoBurst }, { 0.85f, FinishPalette::tobaccoDeep },
                               { 1.0f, FinishPalette::tobaccoEdge } };
    const Stops tobaccoLight { { 0.0f, FinishPalette::tobaccoLight }, { 0.3f, FinishPalette::tobaccoLightClear },
                               { 0.3f, FinishPalette::clearBlack }, { 1.0f, FinishPalette::tobaccoShadow } };

    // ---- Midnight flake: navy radial, metal flake noise, shine, chrome trim along the bottom
    constexpr float midnightEllipseWidth = 0.8f;
    constexpr float midnightEllipseHeight = 1.6f;
    constexpr float midnightCentreX = 0.35f;
    constexpr float midnightCentreY = 0.2f;
    const Stops midnightBase { { 0.0f, FinishPalette::midnightCentre }, { 0.45f, FinishPalette::midnightMiddle },
                               { 1.0f, FinishPalette::midnightEdge } };
    const Stops midnightShine { { 0.0f, FinishPalette::midnightShine }, { 0.25f, FinishPalette::clearWhite },
                                { 0.8f, FinishPalette::clearBlack }, { 1.0f, FinishPalette::midnightShadow } };
    const Stops chromeTrim { { 0.0f, FinishPalette::chromeTop }, { 0.6f, FinishPalette::chromeMiddle },
                             { 1.0f, FinishPalette::chromeBottom } };
    constexpr float chromeTrimHeight = 3.0f;

    // ---- Concert black: piano gloss, a soft diagonal shine, gold pinstripes 4 px in from top and bottom
    const Stops concertBase { { 0.0f, FinishPalette::concertTop }, { 0.35f, FinishPalette::concertMiddle },
                              { 1.0f, FinishPalette::concertBottom } };
    const Stops glossShine { { 0.0f, FinishPalette::clearWhite }, { 0.3f, FinishPalette::clearWhite },
                             { 0.44f, FinishPalette::glossShine }, { 0.5f, FinishPalette::glossShineSoft },
                             { 0.58f, FinishPalette::clearWhite }, { 1.0f, FinishPalette::clearWhite } };
    const Stops goldPinstripe { { 0.0f, FinishPalette::goldClear }, { 0.08f, FinishPalette::goldEdge },
                                { 0.5f, FinishPalette::goldCentre }, { 0.92f, FinishPalette::goldEdge },
                                { 1.0f, FinishPalette::goldClear } };
    constexpr float pinstripeInset = 4.0f;
    constexpr float pinstripeHeight = 1.0f;

    // ---- Sides
    const Stops mapleShade { { 0.0f, FinishPalette::sideShadeLeft }, { 0.5f, FinishPalette::sideShadeMiddle },
                             { 1.0f, FinishPalette::sideShadeRight } };
    const Stops walnutShade { { 0.0f, FinishPalette::walnutShadeLeft }, { 0.5f, FinishPalette::walnutShadeMiddle },
                              { 1.0f, FinishPalette::walnutShadeRight } };
    const Stops tobaccoSideLeft { { 0.0f, FinishPalette::tobaccoSideOuter }, { 0.45f, FinishPalette::tobaccoSideMiddle },
                                  { 1.0f, FinishPalette::tobaccoSideInner } };
    const Stops tobaccoSideRight { { 0.0f, FinishPalette::tobaccoSideInner }, { 0.55f, FinishPalette::tobaccoSideMiddle },
                                   { 1.0f, FinishPalette::tobaccoSideOuter } };
    const Stops concertSideLeft { { 0.0f, FinishPalette::concertSideOuter }, { 0.4f, FinishPalette::concertSideMiddle },
                                  { 1.0f, FinishPalette::concertSideInner } };
    const Stops concertSideRight { { 0.0f, FinishPalette::concertSideInner }, { 0.6f, FinishPalette::concertSideMiddle },
                                   { 1.0f, FinishPalette::concertSideOuter } };

    /** feTurbulence + a feColorMatrix of the board's form: a constant colour (in linearRGB) whose alpha is
        gain x noise red + offset. */
    struct NoiseLayer
    {
        double frequencyX, frequencyY;
        int octaves, seed;
        float red, green, blue, alphaGain, alphaOffset, opacity;
    };
    constexpr int redChannel = 0;
    constexpr float opaque = 1.0f;
    constexpr NoiseLayer brushedGold { 0.002, 0.85, 2, 5, 0.35f, 0.27f, 0.12f, 0.9f, -0.25f, 1.0f };
    constexpr NoiseLayer flamedMaple { 0.045, 0.012, 3, 12, 0.55f, 0.28f, 0.05f, 2.2f, -0.95f, 1.0f };
    constexpr NoiseLayer metalFlake  { 0.95, 0.95, 1, 31, 0.85f, 0.9f, 1.0f, 9.0f, -5.4f, 0.55f };
    constexpr NoiseLayer mapleGrain  { 0.22, 0.006, 3, 3, 0.55f, 0.36f, 0.18f, 1.3f, -0.45f, 1.0f };
    constexpr NoiseLayer walnutGrain { 0.25, 0.008, 4, 9, 0.12f, 0.06f, 0.03f, 1.6f, -0.5f, 1.0f };
    constexpr NoiseLayer rosewoodGrain { 0.3, 0.01, 4, 21, 0.08f, 0.03f, 0.02f, 1.8f, -0.6f, 1.0f };

    // sRGB transfer (the board's filters work in linearRGB, the SVG default)
    constexpr float linearKnee = 0.0031308f;
    constexpr float linearSlope = 12.92f;
    constexpr float gammaScale = 1.055f;
    constexpr float gammaOffset = 0.055f;
    constexpr float inverseGamma = 0.41666667f;   // 1 / 2.4
    constexpr float pixelCentre = 0.5f;
    constexpr float half = 0.5f;
    constexpr float noAlpha = 0.0f;

    float toSrgb (float linear)
    {
        return linear <= linearKnee ? linear * linearSlope : gammaScale * std::pow (linear, inverseGamma) - gammaOffset;
    }

    juce::ColourGradient gradientAlong (const Stops& stops, juce::Point<float> start, juce::Point<float> end, bool isRadial)
    {
        juce::ColourGradient gradient (stops.front().colour, start, stops.back().colour, end, isRadial);
        for (auto stop = std::next (stops.begin()); stop != std::prev (stops.end()); ++stop)   // the ends are set above
            gradient.addColour (stop->position, stop->colour);
        return gradient;
    }

    /** CSS linear-gradient(angle, stops) over the box: the gradient line runs through the centre, as long as the
        box's extent along that angle. */
    void linear (juce::Graphics& g, juce::Rectangle<float> box, float angleDegrees, const Stops& stops)
    {
        const auto radians = juce::degreesToRadians (angleDegrees);
        const juce::Point<float> direction { std::sin (radians), -std::cos (radians) };
        const auto length = std::abs (box.getWidth() * direction.x) + std::abs (box.getHeight() * direction.y);
        const auto reach = direction * (length * half);
        g.setGradientFill (gradientAlong (stops, box.getCentre() - reach, box.getCentre() + reach, false));
        g.fillRect (box);
    }

    /** CSS radial-gradient(ellipse W% H% at X% Y%, stops): a circle gradient squashed into the ellipse. */
    void radial (juce::Graphics& g, juce::Rectangle<float> box, float widthShare, float heightShare,
                 float centreX, float centreY, const Stops& stops)
    {
        const auto radiusX = box.getWidth() * widthShare;
        const auto radiusY = box.getHeight() * heightShare;
        const auto centre = box.getRelativePoint (centreX, centreY);
        juce::FillType fill (gradientAlong (stops, centre, centre.translated (radiusX, noAlpha), true));
        fill.transform = juce::AffineTransform::scale (opaque, radiusY / radiusX, centre.x, centre.y);
        g.setFillType (fill);
        g.fillRect (box);
    }

    void noise (juce::Graphics& g, juce::Rectangle<int> box, float scale, const NoiseLayer& layer)
    {
        if (box.isEmpty())
            return;

        const FractalNoise field (layer.seed);
        const auto colour = juce::Colour::fromFloatRGBA (toSrgb (layer.red), toSrgb (layer.green), toSrgb (layer.blue), opaque);
        juce::Image image (juce::Image::ARGB, box.getWidth(), box.getHeight(), true);
        {
            juce::Image::BitmapData pixels (image, juce::Image::BitmapData::writeOnly);
            for (int row {}; row < box.getHeight(); ++row)
                for (int column {}; column < box.getWidth(); ++column)
                {
                    // Board units: the board's own pixel centres, so the texture keeps its size at any scale.
                    const auto red = field.sample (redChannel, (column + pixelCentre) / scale, (row + pixelCentre) / scale,
                                                   layer.frequencyX, layer.frequencyY, layer.octaves, FractalNoise::Type::fractalNoise);
                    const auto alpha = juce::jlimit (noAlpha, opaque, layer.alphaGain * red + layer.alphaOffset) * layer.opacity;
                    pixels.setPixelColour (column, row, colour.withAlpha (alpha));
                }
        }
        g.drawImageAt (image, box.getX(), box.getY());
    }

    void paintSurface (juce::Graphics& g, PanelFinish::Id finish, juce::Rectangle<int> area, float scale, bool withTrims)
    {
        using Id = PanelFinish::Id;
        const auto box = area.toFloat();
        switch (finish)
        {
            case Id::rossoBurst:
                radial (g, box, rossoEllipseWidth, rossoEllipseHeight, rossoCentreX, rossoCentreY, rossoBurst);
                linear (g, box, toBottom, lacquerShine);
                break;
            case Id::champagne:
                linear (g, box, toBottom, champagneBase);
                noise (g, area, scale, brushedGold);
                linear (g, box, glossAngle, champagneSheen);
                break;
            case Id::tobaccoBurst:
                g.setColour (FinishPalette::tobaccoBase);
                g.fillRect (box);
                noise (g, area, scale, flamedMaple);
                radial (g, box, tobaccoEllipseWidth, tobaccoEllipseHeight, tobaccoCentreX, tobaccoCentreY, tobaccoBurst);
                linear (g, box, toBottom, tobaccoLight);
                break;
            case Id::midnightFlake:
                radial (g, box, midnightEllipseWidth, midnightEllipseHeight, midnightCentreX, midnightCentreY, midnightBase);
                noise (g, area, scale, metalFlake);
                linear (g, box, toBottom, midnightShine);
                if (withTrims)
                    linear (g, box.withTop (box.getBottom() - chromeTrimHeight * scale), toBottom, chromeTrim);
                break;
            case Id::concertBlack:
                linear (g, box, toBottom, concertBase);
                linear (g, box, glossAngle, glossShine);
                if (! withTrims)
                    break;
                linear (g, box.withY (box.getY() + pinstripeInset * scale).withHeight (pinstripeHeight * scale), toRight, goldPinstripe);
                linear (g, box.withY (box.getBottom() - (pinstripeInset + pinstripeHeight) * scale).withHeight (pinstripeHeight * scale),
                        toRight, goldPinstripe);
                break;
            case Id::stage:
                break;
        }
    }

    void paintSide (juce::Graphics& g, PanelFinish::Id finish, juce::Rectangle<int> area, float scale, bool isRightSide)
    {
        using Id = PanelFinish::Id;
        const auto box = area.toFloat();
        auto fillWith = [&g, box] (juce::Colour colour) { g.setColour (colour); g.fillRect (box); };
        switch (finish)
        {
            case Id::rossoBurst:
                fillWith (FinishPalette::mapleBase);
                noise (g, area, scale, mapleGrain);
                linear (g, box, toRight, mapleShade);
                break;
            case Id::champagne:
                fillWith (FinishPalette::walnutBase);
                noise (g, area, scale, walnutGrain);
                linear (g, box, toRight, walnutShade);
                break;
            case Id::tobaccoBurst:
                linear (g, box, toRight, isRightSide ? tobaccoSideRight : tobaccoSideLeft);
                break;
            case Id::midnightFlake:
                fillWith (FinishPalette::rosewoodBase);
                noise (g, area, scale, rosewoodGrain);
                break;
            case Id::concertBlack:
                linear (g, box, toRight, isRightSide ? concertSideRight : concertSideLeft);
                break;
            case Id::stage:
                break;
        }
    }

    // WCAG relative luminance and contrast (sRGB).
    constexpr float srgbKnee = 0.04045f;
    constexpr float srgbScale = 1.055f;
    constexpr float srgbGamma = 2.4f;
    constexpr float redWeight = 0.2126f;
    constexpr float greenWeight = 0.7152f;
    constexpr float blueWeight = 0.0722f;
    constexpr float flare = 0.05f;

    float luminance (juce::Colour colour)
    {
        auto linear = [] (float channel)
            { return channel <= srgbKnee ? channel / linearSlope : std::pow ((channel + gammaOffset) / srgbScale, srgbGamma); };
        return redWeight * linear (colour.getFloatRed()) + greenWeight * linear (colour.getFloatGreen()) + blueWeight * linear (colour.getFloatBlue());
    }

    constexpr float readableContrast = 3.0f;
    constexpr size_t oneStep = 1;   // the labels are small bold caps: 3:1, WCAG's floor for bold text

    juce::Image render (int width, int height, const std::function<void (juce::Graphics&, juce::Rectangle<int>)>& paint)
    {
        if (width <= int {} || height <= int {})
            return {};
        juce::Image image (juce::Image::ARGB, width, height, true);
        juce::Graphics g (image);
        paint (g, image.getBounds());
        return image;
    }
}

namespace PanelFinish
{
    juce::String storedId (Id finish)
    {
        switch (finish)
        {
            case Id::rossoBurst:    return Stored::rossoBurst;
            case Id::champagne:     return Stored::champagne;
            case Id::tobaccoBurst:  return Stored::tobaccoBurst;
            case Id::midnightFlake: return Stored::midnightFlake;
            case Id::concertBlack:  return Stored::concertBlack;
            case Id::stage:         break;
        }
        return Stored::stage;
    }

    Id fromStored (const juce::String& stored)
    {
        for (const auto finish : all)
            if (storedId (finish) == stored)
                return finish;
        return Id::stage;
    }

    juce::String label (Id finish)
    {
        switch (finish)
        {
            case Id::rossoBurst:    return Labels::rossoBurst;
            case Id::champagne:     return Labels::champagne;
            case Id::tobaccoBurst:  return Labels::tobaccoBurst;
            case Id::midnightFlake: return Labels::midnightFlake;
            case Id::concertBlack:  return Labels::concertBlack;
            case Id::stage:         break;
        }
        return Labels::stage;
    }

    juce::Colour inkFor (juce::Colour background, bool bright)
    {
        const auto light = bright ? StagePalette::silkscreenText : StagePalette::silkscreenDimText;
        const auto dark  = bright ? FinishPalette::darkInk : FinishPalette::darkInkDim;
        return contrastRatio (light, background) >= contrastRatio (dark, background) ? light : dark;
    }

    std::array<juce::Rectangle<int>, inkColumnCount> inkColumns (juce::Rectangle<int> content, float scale)
    {
        using namespace StageMetrics;
        auto inner = content.toFloat().reduced (panelPaddingHorizontal * scale, panelPaddingVertical * scale);
        const auto left   = inner.removeFromLeft (panelSideColumnWidth * scale);
        const auto right  = inner.removeFromRight (panelSideColumnWidth * scale);
        return { left.toNearestInt(), inner.toNearestInt(), right.toNearestInt() };
    }

    float contrastRatio (juce::Colour first, juce::Colour second)
    {
        const auto firstLuminance = luminance (first);
        const auto secondLuminance = luminance (second);
        return (juce::jmax (firstLuminance, secondLuminance) + flare) / (juce::jmin (firstLuminance, secondLuminance) + flare);
    }

    juce::Colour inkFor (juce::Colour columnBackground, juce::Colour labelBackground, bool bright)
    {
        const auto columnInk = inkFor (columnBackground, bright);
        return contrastRatio (columnInk, labelBackground) >= readableContrast ? columnInk : inkFor (labelBackground, bright);
    }

    size_t inkColumnAt (const std::array<juce::Rectangle<int>, inkColumnCount>& columns, int labelCentreX)
    {
        size_t column {};
        while (std::next (columns.begin(), (std::ptrdiff_t) column) + oneStep != columns.end()
               && labelCentreX >= columns[column + oneStep].getX())
            ++column;
        return column;
    }

    juce::Colour averageColour (const juce::Image& image, juce::Rectangle<int> area)
    {
        const auto inside = area.getIntersection (image.getBounds());
        if (inside.isEmpty())
            return {};
        float red {}, green {}, blue {};
        int count {};
        const juce::Image::BitmapData pixels (image, juce::Image::BitmapData::readOnly);
        for (int row = inside.getY(); row < inside.getBottom(); ++row)
            for (int column = inside.getX(); column < inside.getRight(); ++column)
            {
                const auto pixel = pixels.getPixelColour (column, row);
                red += pixel.getFloatRed();
                green += pixel.getFloatGreen();
                blue += pixel.getFloatBlue();
                ++count;
            }
        const auto samples = static_cast<float> (count);
        return juce::Colour::fromFloatRGBA (red / samples, green / samples, blue / samples, opaque);
    }

    float sideWidth (float scale) { return sideStripBoardWidth * scale; }

    juce::Image renderSurface (Id finish, int width, int height, float scale, Part part)
    {
        if (finish == Id::stage)
            return {};
        return render (width, height, [finish, scale, part] (juce::Graphics& g, juce::Rectangle<int> area)
                                      { paintSurface (g, finish, area, scale, part == Part::panel); });
    }

    juce::Image renderSide (Id finish, int width, int height, float scale, bool isRightSide)
    {
        if (finish == Id::stage)
            return {};
        return render (width, height, [finish, scale, isRightSide] (juce::Graphics& g, juce::Rectangle<int> area)
                                      { paintSide (g, finish, area, scale, isRightSide); });
    }
}
