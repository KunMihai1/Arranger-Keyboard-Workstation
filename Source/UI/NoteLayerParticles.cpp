#include "NoteLayer.h"
#include "NoteLayerNames.h"
#include <juce_opengl/juce_opengl.h>

#ifndef GL_POINT_SPRITE
#define GL_POINT_SPRITE 0x8861
#endif

// NoteLayer's particle system: shaders, simulation and drawing (split from NoteLayer.cpp, ADR-003). How each style
// looks and moves is ParticleRecipe (pure, unit-tested); this file stores, spawns, steps and draws them.

using namespace juce::gl;
using namespace NoteLayerNames;

namespace
{
    namespace Text
    {
        const juce::String particleShaderFailed = "Shader failed to compile or link:";
    }

    namespace Particles
    {
        constexpr float  dead       = 0.0f;
        constexpr size_t spawnLimit = 1800;
        constexpr float  ndcSpan    = 2.0f;    // pixels to normalised device coordinates (-1..1)
        constexpr float  ndcEdge    = 1.0f;
        constexpr float  aboveKey   = 0.05f;
        constexpr float  diameter   = 2.0f;    // a Dust sprite's radius to its GL point size
    }
}

void NoteLayer::createParticleShader()
{
    std::pair<const char*, const char*> shaderSources = getShaderChoice(renderStyle);
    if (shaderSources.first == nullptr)
        return;

    shader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
    if (!shader->addVertexShader(shaderSources.first) || !shader->addFragmentShader(shaderSources.second) || !shader->link())
    {
        DBG(Text::particleShaderFailed);
        DBG(shader->getLastError());
        jassertfalse;
        shader.reset();
        return;
    }
    positionAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, ShaderNames::position);
    pointSizeAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, ShaderNames::pointSize);
    colourAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, ShaderNames::colour);
    openGLContext.extensions.glGenBuffers(oneBuffer, &particleVBO);
}

void NoteLayer::releaseParticleShader()
{
    if (particleVBO)
    {
        openGLContext.extensions.glDeleteBuffers(oneBuffer, &particleVBO);
        particleVBO = noBuffer;
    }
    positionAttr.reset();
    pointSizeAttr.reset();
    colourAttr.reset();
    shader.reset();
}

void NoteLayer::collectRecipeVertices()
{
    const auto renderingScale = (float) openGLContext.getRenderingScale();
    const int trailPoints = ParticleRecipe::trailPoints(renderStyle);

    for (const auto& particle : particles)
    {
        for (int trailIndex {}; trailIndex < trailPoints; ++trailIndex)
        {
            const auto point = ParticleRecipe::trailPoint(renderStyle, particle, trailIndex);
            if (point.opacity <= Particles::dead)
                break;   // past the start of its trail
            renderVerts.push_back({ point.pos.x, point.pos.y, ParticleRecipe::pointSize(point.size, renderingScale),
                                    particle.colour.getFloatRed(),
                                    particle.colour.getFloatGreen(),
                                    particle.colour.getFloatBlue(),
                                    point.opacity });
        }
    }
}

void NoteLayer::collectDustVertices(float width, float height)
{
    // DustField works in layer pixels (y down); the points go in NDC (y up), sized in physical pixels.
    const auto renderingScale = (float) openGLContext.getRenderingScale();
    const auto colourAlpha = renderParticleColour.getFloatAlpha();

    for (const auto& mote : dust.all())
    {
        const auto sprite = dust.spriteOf(mote);
        const auto opacity = sprite.opacity * colourAlpha;
        if (opacity <= Particles::dead || renderVerts.size() >= renderVerts.capacity())
            continue;
        renderVerts.push_back({ sprite.centre.x / width * Particles::ndcSpan - Particles::ndcEdge,
                                Particles::ndcEdge - sprite.centre.y / height * Particles::ndcSpan,
                                sprite.radius * Particles::diameter * renderingScale,
                                renderParticleColour.getFloatRed(),
                                renderParticleColour.getFloatGreen(),
                                renderParticleColour.getFloatBlue(),
                                opacity });
    }
}

void NoteLayer::drawParticles(float width, float height)
{
    if (shader == nullptr || particleVBO == noBuffer)
        return;

    renderVerts.clear();
    if (renderStyle == ParticleStyles::dust)
        collectDustVertices(width, height);
    else
        collectRecipeVertices();

    if (renderVerts.empty())
        return;

    shader->use();

    float timeSeconds = (float)juce::Time::getMillisecondCounterHiRes() * Timing::secondsPerMillisecondFloat;
    if (shader->getUniformIDFromName(ShaderNames::time) >= Timing::firstUniformId)
        shader->setUniform(ShaderNames::time, timeSeconds);

    auto& ext = openGLContext.extensions;
    ext.glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    ext.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(Vertex)* renderVerts.size()), renderVerts.data(), GL_DYNAMIC_DRAW);

    if (positionAttr != nullptr)
    {
        ext.glVertexAttribPointer(positionAttr->attributeID, Components::xy, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, posX));
        ext.glEnableVertexAttribArray(positionAttr->attributeID);
    }
    if (pointSizeAttr != nullptr)
    {
        ext.glVertexAttribPointer(pointSizeAttr->attributeID, Components::size, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, size));
        ext.glEnableVertexAttribArray(pointSizeAttr->attributeID);
    }
    if (colourAttr != nullptr)
    {
        ext.glVertexAttribPointer(colourAttr->attributeID, Components::rgba, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, red));
        ext.glEnableVertexAttribArray(colourAttr->attributeID);
    }

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_BLEND);
    // Every style adds light: Sparks and Wisps glow over the notes, Dust (the board's 'lighter') under them.
    glBlendFunc(GL_SRC_ALPHA, ParticleRecipe::additive(renderStyle) ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_POINTS, firstVertex, (GLsizei)renderVerts.size());

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   // the notes draw with normal blending
    if (positionAttr) ext.glDisableVertexAttribArray(positionAttr->attributeID);
    if (pointSizeAttr) ext.glDisableVertexAttribArray(pointSizeAttr->attributeID);
    if (colourAttr) ext.glDisableVertexAttribArray(colourAttr->attributeID);
}

std::pair<const char*, const char*> NoteLayer::sparksShader()
{
    const char* sparksVertex =
        "attribute vec2 position;\n"
        "attribute float pointSize;\n"
        "attribute vec4 colour;\n"
        "varying vec4 vColour;\n"
        "void main()\n"
        "{\n"
        "    gl_PointSize = pointSize;\n"
        "    gl_Position = vec4(position, 0.0, 1.0);\n"
        "    vColour = colour;\n"
        "}\n";

    // A glowing spark, the preview's profile (owner picked it there): a bright core over the inner quarter of the
    // radius, white-hot while the spark is young (alpha = life left), then a halo fading evenly to the edge.
    const char* sparksFragment =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 vColour;\n"
        "void main()\n"
        "{\n"
        "    float r = min(1.0, distance(gl_PointCoord.xy, vec2(0.5, 0.5)) * 2.0);\n"
        "    float glow = r < 0.25 ? mix(1.0, 0.85, r / 0.25) : 0.85 * (1.0 - r) / 0.75;\n"
        "    float core = 1.0 - smoothstep(0.0, 0.25, r);\n"
        "    vec3 colour = mix(vColour.rgb, vec3(1.0, 0.98, 0.92), core * vColour.a);\n"
        "    gl_FragColor = vec4(colour, glow * vColour.a);\n"
        "}\n";

    return {sparksVertex, sparksFragment};
}

std::pair<const char*, const char*> NoteLayer::dustShader()
{
    const char* dustVertex =
        "attribute vec2 position;\n"
        "attribute float pointSize;\n"
        "attribute vec4 colour;\n"
        "varying vec4 vColour;\n"
        "void main()\n"
        "{\n"
        "    gl_PointSize = pointSize;\n"
        "    gl_Position = vec4(position, 0.0, 1.0);\n"
        "    vColour = colour;\n"
        "}\n";

    // The board's sprite: a radial gradient, white at the centre -> the particle colour at 18 % of the radius ->
    // that colour at 40 % alpha at 45 % -> transparent at the edge.
    const char* dustFragment =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 vColour;\n"
        "void main()\n"
        "{\n"
        "    float r = min(1.0, distance(gl_PointCoord.xy, vec2(0.5, 0.5)) * 2.0);\n"
        "    vec3 colour = mix(vec3(1.0), vColour.rgb, clamp(r / 0.18, 0.0, 1.0));\n"
        "    float alpha = r < 0.18 ? 1.0 : (r < 0.45 ? mix(1.0, 0.4, (r - 0.18) / 0.27) : mix(0.4, 0.0, (r - 0.45) / 0.55));\n"
        "    gl_FragColor = vec4(colour, alpha * vColour.a);\n"
        "}\n";

    return {dustVertex, dustFragment};
}

std::pair<const char*, const char*> NoteLayer::wispsShader()
{
    const char* wispsVertex =
        "attribute vec2 position;\n"
        "attribute float pointSize;\n"
        "attribute vec4 colour;\n"
        "varying vec4 vColour;\n"
        "void main()\n"
        "{\n"
        "    gl_PointSize = pointSize;\n"
        "    gl_Position = vec4(position, 0.0, 1.0);\n"
        "    vColour = colour;\n"
        "}\n";

    // One point of a wisp's trail: a soft round glow; the points overlap into a smooth line of light.
    const char* wispsFragment =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 vColour;\n"
        "void main()\n"
        "{\n"
        "    float dist = distance(gl_PointCoord.xy, vec2(0.5, 0.5));\n"
        "    float glow = 1.0 - smoothstep(0.15, 0.5, dist);\n"
        "    gl_FragColor = vec4(vColour.rgb, glow * vColour.a);\n"
        "}\n";

    return {wispsVertex, wispsFragment};
}

std::pair<const char*, const char*> NoteLayer::getShaderChoice(int styleId)
{
    switch (styleId)
    {
    case ParticleStyles::sparks: return sparksShader();
    case ParticleStyles::dust: return dustShader();
    case ParticleStyles::wisps: return wispsShader();
    default:
        return {nullptr,nullptr};
    }
}

void NoteLayer::updateParticles(float dt)
{
    // Sparks / Wisps still alive when the style changes to Dust are never drawn and no recipe ages them: drop them,
    // or the layer would keep animating with nothing on screen.
    if (renderStyle == ParticleStyles::dust)
    {
        particles.clear();
        return;
    }

    for (int particleIndex {}; particleIndex < (int)particles.size(); )
    {
        auto& particle = particles[particleIndex];
        ParticleRecipe::step(renderStyle, particle, dt);

        // Unordered erase for O(1) removal, avoiding expensive std::remove_if memory shifting
        if (particle.life <= Particles::dead) {
            particles[particleIndex] = particles.back();
            particles.pop_back();
        } else {
            ++particleIndex;
        }
    }
}

void NoteLayer::spawnParticlesForNote(juce::Rectangle<int> keyBounds, float width, float height)
{
    if (renderStyle == ParticleStyles::dust)
    {
        if (renderSpawnParticles && !keyBounds.isEmpty())   // a note outside the piano has no key to rise from
            dust.burst((float) keyBounds.getCentreX(), (float) keyBounds.getWidth(), rng);
        return;
    }

    if (!renderSpawnParticles || particles.size() > Particles::spawnLimit || width <= emptySize || height <= emptySize)
        return; // Prevent excessive spawning if many notes are played simultaneously

    float x_ndc = (keyBounds.getCentreX() / width) * Particles::ndcSpan - Particles::ndcEdge;
    float y_ndc = -(Particles::ndcEdge - (keyBounds.getY() / height) * Particles::ndcSpan);
    y_ndc += Particles::aboveKey;
    const juce::Point<float> keyTop(x_ndc, y_ndc);

    for (int particleIndex {}, count = ParticleRecipe::perNote(renderStyle); particleIndex < count; ++particleIndex)
        particles.push_back(ParticleRecipe::spawn(renderStyle, keyTop, height / width, renderParticleColour, rng));
}

void NoteLayer::setKeyHeld(int midiNote, juce::Rectangle<int> keyBounds, bool isHeld)
{
    if (! juce::isPositiveAndBelow(midiNote, midiNotes))
        return;
    keyHeld[(size_t) midiNote] = isHeld && ! keyBounds.isEmpty();
    heldKeyBounds[(size_t) midiNote] = keyBounds;
}

void NoteLayer::emitDustForHeldKeys(float dt)
{
    // Board `Dust`: a held key keeps streaming (60 a second), on top of the burst its press made.
    if (renderStyle != ParticleStyles::dust || !renderSpawnParticles || dt <= Timing::noStep)
        return;

    for (size_t midiNote {}; midiNote < keyHeld.size(); ++midiNote)
        if (keyHeld[midiNote])
            dust.emitHeld((float) heldKeyBounds[midiNote].getCentreX(), (float) heldKeyBounds[midiNote].getWidth(), dt, rng);
}
