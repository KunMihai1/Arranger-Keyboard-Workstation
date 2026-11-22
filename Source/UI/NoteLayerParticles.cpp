#include "NoteLayer.h"
#include "NoteLayerNames.h"
#include <juce_opengl/juce_opengl.h>

#ifndef GL_POINT_SPRITE
#define GL_POINT_SPRITE 0x8861
#endif

// NoteLayer's particle system: shaders, simulation and drawing (split from NoteLayer.cpp, ADR-003).

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
        constexpr float  pointSizeScale = 0.5f;
        constexpr float  centred        = 0.5f;    // nextFloat() - centred spreads evenly both ways
        constexpr float  dead           = 0.0f;
        constexpr size_t spawnLimit     = 1800;
        constexpr float  ndcSpan        = 2.0f;    // pixels to normalised device coordinates (-1..1)
        constexpr float  ndcEdge        = 1.0f;
        constexpr float  aboveKey       = 0.05f;
        constexpr float  noSpread       = 0.0f;
    }

    namespace Sparks
    {
        constexpr int   perNote = 25;
        constexpr float gravity = 2.0f;
        constexpr float fadeRate = 1.5f;
        constexpr float spreadVelocity = 0.8f;
        constexpr float riseVelocity = 1.5f;
        constexpr float sizeMin = 2.0f;
        constexpr float sizeSpan = 6.0f;
        constexpr float lifeMin = 0.8f;
        constexpr float lifeSpan = 0.4f;
    }

    namespace Dust
    {
        constexpr int   perNote = 40;
        constexpr float jitter = 0.5f;
        constexpr float dragPerTick = 0.95f;
        constexpr float ticksPerSecond = 60.0f;
        constexpr float fadeRate = 0.6f;
        constexpr float spreadX = 0.08f;
        constexpr float spreadY = 0.02f;
        constexpr float spreadVelocity = 0.2f;
        constexpr float riseSpan = 0.4f;
        constexpr float riseMin = 0.1f;
        constexpr float sizeMin = 3.0f;
        constexpr float sizeSpan = 15.0f;
        constexpr float alphaMin = 0.4f;
        constexpr float alphaSpan = 0.6f;
        constexpr float lifeMin = 0.8f;
        constexpr float lifeSpan = 0.4f;
    }

    namespace Smoke
    {
        constexpr int   perNote = 15;
        constexpr float jitter = 0.2f;
        constexpr float growthRate = 30.0f;
        constexpr float fadeRate = 0.4f;
        constexpr float spreadX = 0.12f;
        constexpr float spreadVelocity = 0.1f;
        constexpr float riseSpan = 0.2f;
        constexpr float riseMin = 0.05f;
        constexpr float sizeMin = 20.0f;
        constexpr float sizeSpan = 30.0f;
        constexpr float alphaMin = 0.15f;
        constexpr float alphaSpan = 0.2f;
        constexpr float lifeMin = 1.0f;
        constexpr float lifeSpan = 1.0f;
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

    for (const auto& particle : particles)
    {
        renderVerts.push_back({ particle.pos.x, particle.pos.y, particle.size*Particles::pointSizeScale,
                          particle.colour.getFloatRed(),
                          particle.colour.getFloatGreen(),
                          particle.colour.getFloatBlue(),
                          particle.colour.getFloatAlpha() * particle.life });
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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_POINTS, firstVertex, (GLsizei)renderVerts.size());

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

    const char* sparksFragment =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 vColour;\n"
        "void main()\n"
        "{\n"
        "    vec2 uv = gl_PointCoord.xy;\n"
        "    float dist = distance(uv, vec2(0.5, 0.5));\n"
        "    float alpha = 1.0 - smoothstep(0.4, 0.5, dist);\n"
        "    gl_FragColor = vec4(vColour.rgb, alpha * vColour.a);\n"
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

std::pair<const char*, const char*> NoteLayer::smokeShader()
{
    const char* smokeVertex =
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

    const char* smokeFragment =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec4 vColour;\n"
        "void main()\n"
        "{\n"
        "    vec2 uv = gl_PointCoord.xy;\n"
        "    float dist = distance(uv, vec2(0.5, 0.5));\n"
        "    float alpha = 1.0 - smoothstep(0.0, 0.5, dist);\n"
        "    alpha = alpha * alpha;\n"
        "    gl_FragColor = vec4(vColour.rgb, alpha * vColour.a);\n"
        "}\n";

    return {smokeVertex, smokeFragment};
}

std::pair<const char*, const char*> NoteLayer::getShaderChoice(int styleId)
{
    switch (styleId)
    {
    case ParticleStyles::sparks: return sparksShader();
    case ParticleStyles::dust: return dustShader();
    case ParticleStyles::smoke: return smokeShader();
    default:
        return {nullptr,nullptr};
    }
}

void NoteLayer::updateParticles(float dt)
{
    for (int particleIndex {}; particleIndex < (int)particles.size(); )
    {
        auto& particle = particles[particleIndex];
        if (renderStyle == ParticleStyles::sparks) { // Sparks
            particle.velocity.y -= Sparks::gravity * dt; // Gravity pulls them down slightly
            particle.pos += particle.velocity * dt;
            particle.life -= dt * Sparks::fadeRate; // Fast fade
        }
        else if (renderStyle == ParticleStyles::dust) { // Dust
            particle.velocity.x += (rng.nextFloat() - Particles::centred) * Dust::jitter * dt; // Brownian
            particle.velocity.x *= std::pow(Dust::dragPerTick, dt * Dust::ticksPerSecond); // Drag: 0.95 per 60 Hz tick, at any frame rate
            particle.pos += particle.velocity * dt;
            particle.life -= dt * Dust::fadeRate; // Slow fade
        }
        else if (renderStyle == ParticleStyles::smoke) { // Smoke
            particle.velocity.x += (rng.nextFloat() - Particles::centred) * Smoke::jitter * dt; // Slight wiggle
            particle.size += dt * Smoke::growthRate; // Smoke expands as it rises
            particle.pos += particle.velocity * dt;
            particle.life -= dt * Smoke::fadeRate; // Very slow fade
        }

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
    juce::Point<float> pos(x_ndc, y_ndc);

    int count {};
    if (renderStyle == ParticleStyles::sparks) count = Sparks::perNote; // Sparks
    else if (renderStyle == ParticleStyles::dust) count = Dust::perNote; // Dust
    else if (renderStyle == ParticleStyles::smoke) count = Smoke::perNote; // Smoke

    for (int particleIndex {}; particleIndex < count; particleIndex++)
    {
        Particle particle;
        if (renderStyle == ParticleStyles::sparks) { // Sparks
            particle.pos = pos;
            particle.velocity = { (rng.nextFloat() - Particles::centred) * Sparks::spreadVelocity, rng.nextFloat() * Sparks::riseVelocity };
            particle.size = Sparks::sizeMin + rng.nextFloat() * Sparks::sizeSpan;
            particle.life = Sparks::lifeMin + rng.nextFloat() * Sparks::lifeSpan;
            particle.colour = renderParticleColour;
        }
        else if (renderStyle == ParticleStyles::dust) { // Dust
            float spreadX = (rng.nextFloat() - Particles::centred) * Dust::spreadX;
            float spreadY = (rng.nextFloat() - Particles::centred) * Dust::spreadY;
            particle.pos = pos + juce::Point<float>(spreadX, spreadY);
            particle.velocity = { (rng.nextFloat() - Particles::centred) * Dust::spreadVelocity, rng.nextFloat() * Dust::riseSpan + Dust::riseMin };
            particle.size = Dust::sizeMin + rng.nextFloat() * Dust::sizeSpan;
            float alpha = Dust::alphaMin + rng.nextFloat() * Dust::alphaSpan;
            particle.colour = renderParticleColour.withAlpha(alpha);
            particle.life = Dust::lifeMin + rng.nextFloat() * Dust::lifeSpan;
        }
        else if (renderStyle == ParticleStyles::smoke) { // Smoke
            float spreadX = (rng.nextFloat() - Particles::centred) * Smoke::spreadX;
            particle.pos = pos + juce::Point<float>(spreadX, Particles::noSpread);
            particle.velocity = { (rng.nextFloat() - Particles::centred) * Smoke::spreadVelocity, rng.nextFloat() * Smoke::riseSpan + Smoke::riseMin };
            particle.size = Smoke::sizeMin + rng.nextFloat() * Smoke::sizeSpan; // Very large
            float alpha = Smoke::alphaMin + rng.nextFloat() * Smoke::alphaSpan;
            particle.colour = renderParticleColour.withAlpha(alpha);
            particle.life = Smoke::lifeMin + rng.nextFloat() * Smoke::lifeSpan; // longer life
        }
        particles.push_back(particle);
    }
}
