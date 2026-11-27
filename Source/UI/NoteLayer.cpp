/*
  ==============================================================================

    NoteLayer.cpp
    Created: 20 May 2025 1:36:54pm
    Author:  Kisuke

  ==============================================================================
*/


#include "NoteLayer.h"
#include <juce_opengl/juce_opengl.h>

#if JUCE_WINDOWS
 // Declared here rather than pulling <dwmapi.h> (and <windows.h>) into a JUCE translation unit.
 extern "C" __declspec(dllimport) long __stdcall DwmFlush();
 #pragma comment(lib, "dwmapi.lib")
#endif

using namespace juce::gl;

#include "NoteLayerNames.h"

using namespace NoteLayerNames;

namespace
{
    namespace Text
    {
        const juce::String glTypePrefix         = "[GL] type=0x";
        const juce::String glSeverityPrefix     = " sev=0x";
        const juce::String glIdPrefix           = " id=";
        const juce::String glMessageSeparator   = " : ";
        const juce::String glErrorLog           = "piano_gl_errors.log";   // on the desktop
        const juce::String noteShaderFailed     = "Note shader failed to compile or link:";
    }

    namespace Capacity
    {
        constexpr size_t particles       = 2000;
        constexpr size_t verticesPerNote = 6;     // two triangles
        constexpr size_t notes           = 128;
        constexpr size_t commands        = 256;
    }

    namespace ParticleColours
    {
        constexpr float goldHue        = 0.12f;
        constexpr float saturationMin  = 0.85f;
        constexpr float saturationSpan = 0.1f;
        constexpr float brightnessMin  = 0.9f;
        constexpr float brightnessSpan = 0.1f;
        constexpr float alphaMin       = 0.6f;
        constexpr float alphaSpan      = 0.4f;
    }

    namespace Notes
    {
        constexpr float  pulseMid          = 0.5f;
        constexpr float  pulseDepth        = 0.5f;
        constexpr float  pulseRadiansPerMs = 0.004f;
        constexpr float  outlineMargin     = 2.0f;
        constexpr float  releasedPulse     = -1.0f;   // tells the shader the note is no longer held
        constexpr float  glowSaturation    = 0.9f;
        constexpr float  glowBrightness    = 1.0f;
        constexpr float  glowAlpha         = 1.0f;
        constexpr double onePixel          = 1.0;
        constexpr double smallestScale     = 0.5;
    }

    constexpr float clearChannel = 0.0f;    // transparent black
}

NoteLayer::NoteLayer(KeyboardUI& referenceKeyboard)
    : keyBoardUI{ referenceKeyboard }
{
    particles.reserve(Capacity::particles);
    renderVerts.reserve(Capacity::particles);
    noteVerts.reserve(Capacity::verticesPerNote * Capacity::notes);
    commands.reserve(Capacity::commands);
    pendingCommands.reserve(Capacity::commands);

    float hue = ParticleColours::goldHue;            // yellow-gold hue
    float saturation = ParticleColours::saturationMin + juce::Random::getSystemRandom().nextFloat() * ParticleColours::saturationSpan;  // 0.85 - 0.95
    float brightness = ParticleColours::brightnessMin + juce::Random::getSystemRandom().nextFloat() * ParticleColours::brightnessSpan;   // 0.9 - 1.0
    float alpha = ParticleColours::alphaMin + juce::Random::getSystemRandom().nextFloat() * ParticleColours::alphaSpan;        // 0.6 - 1.0
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

void NoteLayer::setHeldHidden(bool shouldStayHidden)
{
    heldHidden = shouldStayHidden;
}

void NoteLayer::handleNoteOn(int midiNote)
{
    if (!isVisible() && !heldHidden && keyBoardUI.isVisible())
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
    lastFrameTime = {};
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
            juce::String line = Text::glTypePrefix + juce::String::toHexString((int) type)
                              + Text::glSeverityPrefix + juce::String::toHexString((int) severity)
                              + Text::glIdPrefix + juce::String((int) id) + Text::glMessageSeparator + juce::String(message);
            if (line == lastLogged)
                return; // collapse the same error repeating every frame
            lastLogged = line;

            DBG(line);
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                .getChildFile(Text::glErrorLog)
                .appendText(line + juce::newLine);
        }, nullptr);
    }
#endif
}

void NoteLayer::openGLContextClosing()
{
    releaseParticleShader();

    if (noteVBO != noBuffer)
    {
        openGLContext.extensions.glDeleteBuffers(oneBuffer, &noteVBO);
        noteVBO = noBuffer;
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

    const double now = juce::Time::getMillisecondCounterHiRes() * Timing::secondsPerMillisecond;
    const float dt = lastFrameTime > Timing::noFrameYet ? (float) std::min(Timing::longestStepSeconds, now - lastFrameTime) : Timing::noStep;
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

    glClearColor(clearChannel, clearChannel, clearChannel, clearChannel);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (width > emptySize && height > emptySize)
    {
        // Dust under the notes (as the old juce::Graphics notes were composited over it); Sparks and Smoke
        // over them, so they read as light and smoke in front of the notes, not haze behind them.
        if (ParticleRecipe::drawnOverNotes(renderStyle))
        {
            drawNotes(width, height);
            drawParticles();
        }
        else
        {
            drawParticles();
            drawNotes(width, height);
        }
    }

    // Keep drawing while anything moves (DwmFlush paces it), then go idle until the next command.
    animating = !field.isIdle() || !particles.empty();
    if (animating)
        openGLContext.triggerRepaint();
    else
        lastFrameTime = {};
}

void NoteLayer::applyCommands(float width, float height)
{
    for (const auto& command : pendingCommands)
    {
        switch (command.type)
        {
        case Command::Type::noteOn:
            if (field.noteOn(command.midiNote, (float) command.keyBounds.getX(), (float) command.keyBounds.getWidth()))
                spawnParticlesForNote(command.keyBounds, width, height);
            break;
        case Command::Type::noteOff:    field.noteOff(command.midiNote); break;
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
        DBG(Text::noteShaderFailed);
        DBG(noteShader->getLastError());
        jassertfalse;
        noteShader.reset();
        return;
    }
    notePositionAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*noteShader, ShaderNames::position);
    noteLocalAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*noteShader, ShaderNames::local);
    noteRectAttr = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*noteShader, ShaderNames::rect);
    openGLContext.extensions.glGenBuffers(oneBuffer, &noteVBO);
}

void NoteLayer::drawNotes(float width, float height)
{
    if (noteShader == nullptr || noteVBO == noBuffer || field.isIdle())
        return;

    // "Still held" cue: the amber glow breathes in unison across a held chord.
    const float heldPulse = Notes::pulseMid + Notes::pulseDepth * std::sin((float) juce::Time::getMillisecondCounterHiRes() * Notes::pulseRadiansPerMs);

    noteVerts.clear();
    auto addQuad = [this](const NoteField::Note& note, float pulse)
    {
        const float margin = Notes::outlineMargin; // margin so the outline's antialiased outer edge isn't clipped
        const float x0 = note.left - margin, x1 = note.left + note.width + margin, y0 = note.top - margin, y1 = note.top + note.height + margin;
        const NoteVertex topLeft     { x0, y0, -margin, -margin, note.width, note.height, note.alpha, pulse };
        const NoteVertex topRight    { x1, y0, note.width + margin, -margin, note.width, note.height, note.alpha, pulse };
        const NoteVertex bottomLeft  { x0, y1, -margin, note.height + margin, note.width, note.height, note.alpha, pulse };
        const NoteVertex bottomRight { x1, y1, note.width + margin, note.height + margin, note.width, note.height, note.alpha, pulse };
        noteVerts.insert(noteVerts.end(), { topLeft, topRight, bottomLeft, topRight, bottomRight, bottomLeft });
    };

    for (const auto& held : field.held())
        addQuad(held.second, heldPulse);
    for (const auto& released : field.falling())
        addQuad(released, Notes::releasedPulse);

    noteShader->use();
    noteShader->setUniform(ShaderNames::viewport, width, height);
    const auto glow = juce::Colour::fromHSV(ParticleColours::goldHue, Notes::glowSaturation, Notes::glowBrightness, Notes::glowAlpha); // app gold/amber
    noteShader->setUniform(ShaderNames::glowRgb, glow.getFloatRed(), glow.getFloatGreen(), glow.getFloatBlue());
    noteShader->setUniform(ShaderNames::antialiasWidth, (float) (Notes::onePixel / std::max(Notes::smallestScale, openGLContext.getRenderingScale())));

    auto& ext = openGLContext.extensions;
    ext.glBindBuffer(GL_ARRAY_BUFFER, noteVBO);
    ext.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) (sizeof(NoteVertex) * noteVerts.size()), noteVerts.data(), GL_DYNAMIC_DRAW);
    ext.glVertexAttribPointer(notePositionAttr->attributeID, Components::xy, GL_FLOAT, GL_FALSE, sizeof(NoteVertex), (GLvoid*) offsetof(NoteVertex, px));
    ext.glVertexAttribPointer(noteLocalAttr->attributeID, Components::xy, GL_FLOAT, GL_FALSE, sizeof(NoteVertex), (GLvoid*) offsetof(NoteVertex, lx));
    ext.glVertexAttribPointer(noteRectAttr->attributeID, Components::rect, GL_FLOAT, GL_FALSE, sizeof(NoteVertex), (GLvoid*) offsetof(NoteVertex, rectWidth));
    for (auto* attr : { notePositionAttr.get(), noteLocalAttr.get(), noteRectAttr.get() })
        ext.glEnableVertexAttribArray(attr->attributeID);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, firstVertex, (GLsizei) noteVerts.size());

    for (auto* attr : { notePositionAttr.get(), noteLocalAttr.get(), noteRectAttr.get() })
        ext.glDisableVertexAttribArray(attr->attributeID);
}
