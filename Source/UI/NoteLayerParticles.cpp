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

void NoteLayer::drawParticles()
{
    if (shader == nullptr || particleVBO == noBuffer || particles.empty())
        return;

    shader->use();

    float timeSeconds = (float)juce::Time::getMillisecondCounterHiRes() * Timing::secondsPerMillisecondFloat;
    if (shader->getUniformIDFromName(ShaderNames::time) >= Timing::firstUniformId)
        shader->setUniform(ShaderNames::time, timeSeconds);

    renderVerts.clear();
    const auto renderingScale = (float) openGLContext.getRenderingScale();
    const int trailPoints = ParticleRecipe::trailPoints(renderStyle);

    for (const auto& particle : particles)
    {
        for (int trailIndex {}; trailIndex < trailPoints; ++trailIndex)
        {
            const auto point = ParticleRecipe::trailPoint(renderStyle, particle, trailIndex);
            if (point.opacity <= Particles::dead)
                break;   // past the start of its trail
            renderVerts.push_back({ point.pos.x, point.pos.y, ParticleRecipe::pointSize(renderStyle, point.size, renderingScale),
                                    particle.colour.getFloatRed(),
                                    particle.colour.getFloatGreen(),
                                    particle.colour.getFloatBlue(),
                                    point.opacity });
        }
    }

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
    // Sparks and wisps add light (a glow over the notes); Dust covers what is under it.
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

    // A glowing spark: a soft halo, and a core that is white-hot while the spark is young (alpha = life left).
    const char* sparksFragment =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 vColour;\n"
        "void main()\n"
        "{\n"
        "    float dist = distance(gl_PointCoord.xy, vec2(0.5, 0.5));\n"
        "    float glow = exp(-dist * dist * 14.0);\n"
        "    float core = 1.0 - smoothstep(0.0, 0.2, dist);\n"
        "    vec3 colour = mix(vColour.rgb, vec3(1.0), core * vColour.a);\n"
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

    const char* dustFragment =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 vColour;\n"
        "void main()\n"
        "{\n"
        "    vec2 uv = gl_PointCoord.xy;\n"
        "    float dist = distance(uv, vec2(0.5, 0.5));\n"
        "    float alpha = 1.0 - smoothstep(0.0, 0.5, dist);\n"
        "    float core = 1.0 - smoothstep(0.0, 0.2, dist);\n"
        "    vec3 color = vColour.rgb + vec3(core * 0.3);\n"
        "    gl_FragColor = vec4(color, alpha * vColour.a);\n"
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
        "    float glow = exp(-dist * dist * 16.0);\n"
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
    for (int particleIndex {}; particleIndex < (int)particles.size(); )
    {
        auto& particle = particles[particleIndex];
        ParticleRecipe::step(renderStyle, particle, dt, rng);

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
    if (!renderSpawnParticles || particles.size() > Particles::spawnLimit || width <= emptySize || height <= emptySize)
        return; // Prevent excessive spawning if many notes are played simultaneously

    float x_ndc = (keyBounds.getCentreX() / width) * Particles::ndcSpan - Particles::ndcEdge;
    float y_ndc = -(Particles::ndcEdge - (keyBounds.getY() / height) * Particles::ndcSpan);
    y_ndc += Particles::aboveKey;
    const juce::Point<float> keyTop(x_ndc, y_ndc);

    for (int particleIndex {}, count = ParticleRecipe::perNote(renderStyle); particleIndex < count; ++particleIndex)
        particles.push_back(ParticleRecipe::spawn(renderStyle, keyTop, height / width, renderParticleColour, rng));
}
