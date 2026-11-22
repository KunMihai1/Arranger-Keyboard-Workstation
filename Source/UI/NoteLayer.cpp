/*
  ==============================================================================

    NoteLayer.cpp
    Created: 20 May 2025 1:36:54pm
    Author:  Kisuke

  ==============================================================================
*/


#include "NoteLayer.h"
#include <juce_opengl/juce_opengl.h>

#ifndef GL_POINT_SPRITE
#define GL_POINT_SPRITE 0x8861
#endif

#if JUCE_WINDOWS
 // Declared here rather than pulling <dwmapi.h> (and <windows.h>) into a JUCE translation unit.
 extern "C" __declspec(dllimport) long __stdcall DwmFlush();
 #pragma comment(lib, "dwmapi.lib")
#endif

using namespace juce::gl;

NoteLayer::NoteLayer(KeyboardUI& referenceKeyboard)
    : keyBoardUI{ referenceKeyboard }
{
    particles.reserve(2000);
    renderVerts.reserve(2000);
    noteVerts.reserve(6 * 128);
    commands.reserve(256);
    pendingCommands.reserve(256);

    float hue = 0.12f;            // yellow-gold hue
    float saturation = 0.85f + juce::Random::getSystemRandom().nextFloat() * 0.1f;  // 0.85 - 0.95
    float brightness = 0.9f + juce::Random::getSystemRandom().nextFloat() * 0.1f;   // 0.9 - 1.0
    float alpha = 0.6f + juce::Random::getSystemRandom().nextFloat() * 0.4f;        // 0.6 - 1.0
    this->particleColourUser = juce::Colour::fromHSV(hue, saturation, brightness, alpha);

    // Draw everything ourselves in renderOpenGL(). With component painting on, JUCE takes the
    // message-thread lock for every frame; with it off, the render thread never needs that lock.
    openGLContext.setRenderer(this);
    openGLContext.setComponentPaintingEnabled(false);
    openGLContext.attachTo(*this);
}

NoteLayer::~NoteLayer()
{
    openGLContext.detach();
}

//==============================================================================
// Message thread

void NoteLayer::post(Command command)
{
    {
        const std::scoped_lock lock(commandLock);

        // While the layer is hidden its GL context is detached and nothing drains the queue.
        // Every path that shows it again calls resetState() anyway, so collapse rather than grow.
        if (commands.size() >= maxQueuedCommands)
        {
            commands.clear();
            command = { Command::Type::clear };
        }

        commands.push_back(command);
    }

    openGLContext.triggerRepaint(); // thread-safe; a no-op while detached
}

void NoteLayer::noteOnReceived(int midiNote)
{
    // May arrive on the MIDI thread. Hop to the message thread (as before) to read KeyboardUI
    // and to keep note-on/note-off ordering identical to the order they were received in.
    juce::MessageManager::callAsync([safe = safeThis, midiNote]
        {
            if (safe != nullptr)
                safe->handleNoteOn(midiNote);
        });
}

void NoteLayer::handleNoteOn(int midiNote)
{
    if (!isVisible() && keyBoardUI.isVisible())
        setVisible(true);

    const auto key = keyBoardUI.keys.find(midiNote);
    const auto bounds = key != keyBoardUI.keys.end() ? key->second.bounds : juce::Rectangle<int>();
    post({ Command::Type::noteOn, midiNote, bounds });
}

void NoteLayer::noteOffReceived(int midiNote)
{
    juce::MessageManager::callAsync([safe = safeThis, midiNote]
        {
            if (safe != nullptr)
                safe->post({ Command::Type::noteOff, midiNote });
        });
}

void NoteLayer::resetState()
{
    post({ Command::Type::clear });
}

void NoteLayer::resetStateActiveNotes()
{
    post({ Command::Type::releaseAll });
}

void NoteLayer::setColourParticle(juce::Colour& colour)
{
    const std::scoped_lock lock(commandLock);
    particleColourUser = colour;
}

void NoteLayer::setSpawnParticleState(bool state)
{
    const std::scoped_lock lock(commandLock);
    spawnParticleState = state;
}

void NoteLayer::setParticleStyle(int styleId)
{
    const std::scoped_lock lock(commandLock);
    if (currentStyle == styleId) return;
    currentStyle = styleId;
    styleChanged = true;
}

void NoteLayer::resized()
{
    // Do NOT touch OpenGL here: resized() runs on the message thread, while the context is owned by
    // the dedicated GL render thread (calling makeActive()/glViewport() here caused the 2026-08-09
    // GL_DEBUG_TYPE_ERROR crash). JUCE sets the viewport itself every frame; just hand over the size.
    {
        const std::scoped_lock lock(commandLock);
        layerWidth = (float) getWidth();
        layerHeight = (float) getHeight();
    }
    openGLContext.triggerRepaint();
}

//==============================================================================
// Render thread

void NoteLayer::newOpenGLContextCreated()
{
    // The context is destroyed whenever the layer is hidden and created again when it is shown.
    lastFrameTime = 0.0;
    animating = false;

    createParticleShader();
    createNoteShader();

#if JUCE_DEBUG
    // --- GL error tripwire (permanent) ---
    // Replace JUCE's assert-only GL debug callback (juce_OpenGLContext.cpp:662, which *breaks/crashes*
    // the app on any driver-reported GL error) with one that instead records the error to a file. This
    // turns the rare, timing-dependent "crash" into a benign logged event: if a real GL error ever
    // recurs, the exact line is captured in Desktop/piano_gl_errors.log with zero debugger effort.
    // Runs on the render thread (single thread per context), so the static dedup state needs no lock.
    // Skips notifications / perf hints, incl. the harmless NVIDIA "will use VIDEO memory" spam.
    if (glDebugMessageCallback != nullptr)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback([](GLenum, GLenum type, GLuint id, GLenum severity, GLsizei,
                                  const GLchar* message, const void*)
        {
            if (type != GL_DEBUG_TYPE_ERROR && severity != GL_DEBUG_SEVERITY_HIGH)
                return; // notifications / perf hints (incl. the "video memory" spam) -- ignore

            static juce::String lastLogged;
            juce::String line = "[GL] type=0x" + juce::String::toHexString((int) type)
                              + " sev=0x" + juce::String::toHexString((int) severity)
                              + " id=" + juce::String((int) id) + " : " + juce::String(message);
            if (line == lastLogged)
                return; // collapse the same error repeating every frame
            lastLogged = line;

            DBG(line);
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                .getChildFile("piano_gl_errors.log")
                .appendText(line + juce::newLine);
        }, nullptr);
    }
#endif
}

void NoteLayer::openGLContextClosing()
{
    releaseParticleShader();

    if (noteVBO != 0)
    {
        openGLContext.extensions.glDeleteBuffers(1, &noteVBO);
        noteVBO = 0;
    }
    notePositionAttr.reset();
    noteLocalAttr.reset();
    noteRectAttr.reset();
    noteShader.reset();
}

void NoteLayer::renderOpenGL()
{
#if JUCE_WINDOWS
    // Pace the animation on DWM's composition clock. Waiting in SwapBuffers for vsync instead makes
    // the NVIDIA driver spin: measured ~30 % of a core vs ~2-3 % with this, same 60 fps.
    if (animating)
        DwmFlush();
#endif

    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt = lastFrameTime > 0.0 ? (float) std::min(0.05, now - lastFrameTime) : 0.0f;
    lastFrameTime = now;

    float width, height;
    bool recompileParticles;
    {
        const std::scoped_lock lock(commandLock);
        pendingCommands.swap(commands);
        width = layerWidth;
        height = layerHeight;
        renderParticleColour = particleColourUser;
        renderSpawnParticles = spawnParticleState;
        renderStyle = currentStyle;
        recompileParticles = std::exchange(styleChanged, false);
    }

    if (recompileParticles)
    {
        releaseParticleShader();
        createParticleShader();
    }

    applyCommands(width, height);
    field.setHeight(height);
    field.advance(dt);
    updateParticles(dt);

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (width > 0.0f && height > 0.0f)
    {
        // Particles first, notes on top: the old juce::Graphics notes were composited over
        // whatever renderOpenGL() drew, so this keeps the same layering.
        drawParticles();
        drawNotes(width, height);
    }

    // Keep drawing while anything moves (DwmFlush paces it), then go idle until the next command.
    animating = !field.isIdle() || !particles.empty();
    if (animating)
        openGLContext.triggerRepaint();
    else
        lastFrameTime = 0.0;
}

void NoteLayer::applyCommands(float width, float height)
{
    for (const auto& c : pendingCommands)
    {
        switch (c.type)
        {
        case Command::Type::noteOn:
            if (field.noteOn(c.midiNote, (float) c.keyBounds.getX(), (float) c.keyBounds.getWidth()))
                spawnParticlesForNote(c.keyBounds, width, height);
            break;
        case Command::Type::noteOff:    field.noteOff(c.midiNote); break;
        case Command::Type::releaseAll: field.releaseAll(); break;
        case Command::Type::clear:      field.clear(); particles.clear(); break;
        }
    }
    pendingCommands.clear();
}

void NoteLayer::createNoteShader()
{
    // One rounded rectangle per note, drawn as a quad. The fragment shader reproduces the former
    // juce::Graphics look: white 0.4 -> 0.05 vertical gradient, 1.5 px whitesmoke outline, and on
    // held notes an amber glow band (<= 16 px) plus a 2.5 px top line that pulse together.
    const char* vertex =
        "attribute vec2 position;\n"
        "attribute vec2 local;\n"
        "attribute vec4 rect;\n"
        "uniform vec2 viewport;\n"
        "varying vec2 vLocal;\n"
        "varying vec4 vRect;\n"
        "void main()\n"
        "{\n"
        "    vLocal = local;\n"
        "    vRect = rect;\n"
        "    gl_Position = vec4(position.x / viewport.x * 2.0 - 1.0, 1.0 - position.y / viewport.y * 2.0, 0.0, 1.0);\n"
        "}\n";

    const char* fragment =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "varying vec2 vLocal;\n"
        "varying vec4 vRect;\n"
        "uniform vec3 glowRgb;\n"
        "uniform float aa;\n"
        "float sdRound(vec2 p, vec2 halfSize, float r)\n"
        "{\n"
        "    vec2 q = abs(p) - halfSize + vec2(r);\n"
        "    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;\n"
        "}\n"
        "vec4 over(vec4 s, vec4 d)\n"
        "{\n"
        "    float a = s.a + d.a * (1.0 - s.a);\n"
        "    if (a <= 0.0) return vec4(0.0);\n"
        "    return vec4((s.rgb * s.a + d.rgb * d.a * (1.0 - s.a)) / a, a);\n"
        "}\n"
        "void main()\n"
        "{\n"
        "    float w = vRect.x, h = vRect.y, alpha = vRect.z, pulse = vRect.w;\n"
        "    float r = min(w, h) * 0.25;\n"
        "    float d = sdRound(vLocal - vec2(w, h) * 0.5, vec2(w, h) * 0.5, r);\n"
        "    float ty = clamp(vLocal.y / max(h, 1.0), 0.0, 1.0);\n"
        "    vec4 col = vec4(1.0, 1.0, 1.0, mix(0.4, 0.05, ty) * alpha * clamp(0.5 - d / aa, 0.0, 1.0));\n"
        "    col = over(vec4(0.9608, 0.9608, 0.9608, 0.9 * alpha * clamp((0.75 - abs(d)) / aa + 0.5, 0.0, 1.0)), col);\n"
        "    if (pulse >= 0.0)\n"
        "    {\n"
        "        float gh = min(h, 16.0);\n"
        "        float dg = sdRound(vLocal - vec2(w * 0.5, gh * 0.5), vec2(w, gh) * 0.5, min(r, min(w, gh) * 0.5));\n"
        "        float ga = (0.18 + 0.55 * pulse) * (1.0 - clamp(vLocal.y / gh, 0.0, 1.0));\n"
        "        col = over(vec4(glowRgb, ga * clamp(0.5 - dg / aa, 0.0, 1.0)), col);\n"
        "        float dl = sdRound(vLocal - vec2(w * 0.5, 1.25), vec2(w, 2.5) * 0.5, 1.25);\n"
        "        col = over(vec4(glowRgb, (0.35 + 0.55 * pulse) * clamp(0.5 - dl / aa, 0.0, 1.0)), col);\n"
        "    }\n"
        "    gl_FragColor = col;\n"
        "}\n";

    noteShader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
    if (!noteShader->addVertexShader(vertex) || !noteShader->addFragmentShader(fragment) || !noteShader->link())
    {
        DBG("Note shader failed to compile or link:");
        DBG(noteShader->getLastError());
        jassertfalse;
        noteShader.reset();
        return;
    }
    notePositionAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*noteShader, "position");
    noteLocalAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*noteShader, "local");
    noteRectAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*noteShader, "rect");
    openGLContext.extensions.glGenBuffers(1, &noteVBO);
}

void NoteLayer::drawNotes(float width, float height)
{
    if (noteShader == nullptr || noteVBO == 0 || field.isIdle())
        return;

    // "Still held" cue: the amber glow breathes in unison across a held chord.
    const float heldPulse = 0.5f + 0.5f * std::sin((float) juce::Time::getMillisecondCounterHiRes() * 0.004f);

    noteVerts.clear();
    auto addQuad = [this](const NoteField::Note& n, float pulse)
    {
        const float m = 2.0f; // margin so the outline's antialiased outer edge isn't clipped
        const float x0 = n.left - m, x1 = n.left + n.width + m, y0 = n.top - m, y1 = n.top + n.height + m;
        const NoteVertex a { x0, y0, -m, -m, n.width, n.height, n.alpha, pulse };
        const NoteVertex b { x1, y0, n.width + m, -m, n.width, n.height, n.alpha, pulse };
        const NoteVertex c { x0, y1, -m, n.height + m, n.width, n.height, n.alpha, pulse };
        const NoteVertex d { x1, y1, n.width + m, n.height + m, n.width, n.height, n.alpha, pulse };
        noteVerts.insert(noteVerts.end(), { a, b, c, b, d, c });
    };

    for (const auto& held : field.held())
        addQuad(held.second, heldPulse);
    for (const auto& released : field.falling())
        addQuad(released, -1.0f);

    noteShader->use();
    noteShader->setUniform("viewport", width, height);
    const auto glow = juce::Colour::fromHSV(0.12f, 0.9f, 1.0f, 1.0f); // app gold/amber
    noteShader->setUniform("glowRgb", glow.getFloatRed(), glow.getFloatGreen(), glow.getFloatBlue());
    noteShader->setUniform("aa", (float) (1.0 / std::max(0.5, openGLContext.getRenderingScale())));

    auto& ext = openGLContext.extensions;
    ext.glBindBuffer(GL_ARRAY_BUFFER, noteVBO);
    ext.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (sizeof(NoteVertex) * noteVerts.size()), noteVerts.data(), GL_DYNAMIC_DRAW);
    ext.glVertexAttribPointer(notePositionAttr->attributeID, 2, GL_FLOAT, GL_FALSE, sizeof(NoteVertex), (GLvoid*) offsetof(NoteVertex, px));
    ext.glVertexAttribPointer(noteLocalAttr->attributeID, 2, GL_FLOAT, GL_FALSE, sizeof(NoteVertex), (GLvoid*) offsetof(NoteVertex, lx));
    ext.glVertexAttribPointer(noteRectAttr->attributeID, 4, GL_FLOAT, GL_FALSE, sizeof(NoteVertex), (GLvoid*) offsetof(NoteVertex, w));
    for (auto* attr : { notePositionAttr.get(), noteLocalAttr.get(), noteRectAttr.get() })
        ext.glEnableVertexAttribArray(attr->attributeID);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei) noteVerts.size());

    for (auto* attr : { notePositionAttr.get(), noteLocalAttr.get(), noteRectAttr.get() })
        ext.glDisableVertexAttribArray(attr->attributeID);
}

void NoteLayer::createParticleShader()
{
    std::pair<const char*, const char*> p = getShaderChoice(renderStyle);
    if (p.first == nullptr)
        return;

    shader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
    if (!shader->addVertexShader(p.first) || !shader->addFragmentShader(p.second) || !shader->link())
    {
        DBG("Shader failed to compile or link:");
        DBG(shader->getLastError());
        jassertfalse;
        shader.reset();
        return;
    }
    positionAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "position");
    pointSizeAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "pointSize");
    colourAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "colour");
    openGLContext.extensions.glGenBuffers(1, &particleVBO);
}

void NoteLayer::releaseParticleShader()
{
    if (particleVBO)
    {
        openGLContext.extensions.glDeleteBuffers(1, &particleVBO);
        particleVBO = 0;
    }
    positionAttr.reset();
    pointSizeAttr.reset();
    colourAttr.reset();
    shader.reset();
}

void NoteLayer::drawParticles()
{
    if (shader == nullptr || particleVBO == 0 || particles.empty())
        return;

    shader->use();

    float t = (float)juce::Time::getMillisecondCounterHiRes() * 0.001f;
    if (shader->getUniformIDFromName("uTime") >= 0)
        shader->setUniform("uTime", t);

    renderVerts.clear();

    for (const auto& p : particles)
    {
        renderVerts.push_back({ p.pos.x, p.pos.y, p.size*0.5f,
                          p.colour.getFloatRed(),
                          p.colour.getFloatGreen(),
                          p.colour.getFloatBlue(),
                          p.colour.getFloatAlpha() * p.life });
    }

    auto& ext = openGLContext.extensions;
    ext.glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    ext.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(Vertex)* renderVerts.size()), renderVerts.data(), GL_DYNAMIC_DRAW);

    if (positionAttr != nullptr)
    {
        ext.glVertexAttribPointer(positionAttr->attributeID, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, x));
        ext.glEnableVertexAttribArray(positionAttr->attributeID);
    }
    if (pointSizeAttr != nullptr)
    {
        ext.glVertexAttribPointer(pointSizeAttr->attributeID, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, size));
        ext.glEnableVertexAttribArray(pointSizeAttr->attributeID);
    }
    if (colourAttr != nullptr)
    {
        ext.glVertexAttribPointer(colourAttr->attributeID, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, r));
        ext.glEnableVertexAttribArray(colourAttr->attributeID);
    }

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDrawArrays(GL_POINTS, 0, (GLsizei)renderVerts.size());

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
    case 1: return sparksShader();
    case 2: return dustShader();
    case 3: return smokeShader();
    default:
        return {nullptr,nullptr};
    }
}

void NoteLayer::updateParticles(float dt)
{
    for (int i = 0; i < (int)particles.size(); )
    {
        auto& p = particles[i];
        if (renderStyle == 1) { // Sparks
            p.velocity.y -= 2.0f * dt; // Gravity pulls them down slightly
            p.pos += p.velocity * dt;
            p.life -= dt * 1.5f; // Fast fade
        }
        else if (renderStyle == 2) { // Dust
            p.velocity.x += (rng.nextFloat() - 0.5f) * 0.5f * dt; // Brownian
            p.velocity.x *= std::pow(0.95f, dt * 60.0f); // Drag: 0.95 per 60 Hz tick, at any frame rate
            p.pos += p.velocity * dt;
            p.life -= dt * 0.6f; // Slow fade
        }
        else if (renderStyle == 3) { // Smoke
            p.velocity.x += (rng.nextFloat() - 0.5f) * 0.2f * dt; // Slight wiggle
            p.size += dt * 30.0f; // Smoke expands as it rises
            p.pos += p.velocity * dt;
            p.life -= dt * 0.4f; // Very slow fade
        }

        // Unordered erase for O(1) removal, avoiding expensive std::remove_if memory shifting
        if (p.life <= 0.0f) {
            particles[i] = particles.back();
            particles.pop_back();
        } else {
            ++i;
        }
    }
}

void NoteLayer::spawnParticlesForNote(juce::Rectangle<int> keyBounds, float width, float height)
{
    if (!renderSpawnParticles || particles.size() > 1800 || width <= 0.0f || height <= 0.0f)
        return; // Prevent excessive spawning if many notes are played simultaneously

    float x_ndc = (keyBounds.getCentreX() / width) * 2.0f - 1.0f;
    float y_ndc = -(1.0f - (keyBounds.getY() / height) * 2.0f);
    y_ndc += 0.05f;
    juce::Point<float> pos(x_ndc, y_ndc);

    int count = 0;
    if (renderStyle == 1) count = 25; // Sparks
    else if (renderStyle == 2) count = 40; // Dust
    else if (renderStyle == 3) count = 15; // Smoke

    for (int i = 0; i < count; i++)
    {
        Particle p;
        if (renderStyle == 1) { // Sparks
            p.pos = pos;
            p.velocity = { (rng.nextFloat() - 0.5f) * 0.8f, rng.nextFloat() * 1.5f };
            p.size = 2.0f + rng.nextFloat() * 6.0f;
            p.life = 0.8f + rng.nextFloat() * 0.4f;
            p.colour = renderParticleColour;
        }
        else if (renderStyle == 2) { // Dust
            float spreadX = (rng.nextFloat() - 0.5f) * 0.08f;
            float spreadY = (rng.nextFloat() - 0.5f) * 0.02f;
            p.pos = pos + juce::Point<float>(spreadX, spreadY);
            p.velocity = { (rng.nextFloat() - 0.5f) * 0.2f, rng.nextFloat() * 0.4f + 0.1f };
            p.size = 3.0f + rng.nextFloat() * 15.0f;
            float alpha = 0.4f + rng.nextFloat() * 0.6f;
            p.colour = renderParticleColour.withAlpha(alpha);
            p.life = 0.8f + rng.nextFloat() * 0.4f;
        }
        else if (renderStyle == 3) { // Smoke
            float spreadX = (rng.nextFloat() - 0.5f) * 0.12f;
            p.pos = pos + juce::Point<float>(spreadX, 0.0f);
            p.velocity = { (rng.nextFloat() - 0.5f) * 0.1f, rng.nextFloat() * 0.2f + 0.05f };
            p.size = 20.0f + rng.nextFloat() * 30.0f; // Very large
            float alpha = 0.15f + rng.nextFloat() * 0.2f;
            p.colour = renderParticleColour.withAlpha(alpha);
            p.life = 1.0f + rng.nextFloat() * 1.0f; // longer life
        }
        particles.push_back(p);
    }
}
