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

using namespace juce::gl;

NoteLayer::NoteLayer(KeyboardUI& referenceKeyboard) 
    : keyBoardUI{ referenceKeyboard }, 
      needsShaderRecompile (false)
{
    particles.reserve(2000);
    renderVerts.reserve(2000);

    setSize(getWidth(), getHeight());
    //keyBoardUI.midiHandler.addListener(this);

    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);

    isActive = true;
    float hue = 0.12f;            // yellow-gold hue
    float saturation = 0.85f + juce::Random::getSystemRandom().nextFloat() * 0.1f;  // 0.85 - 0.95
    float brightness = 0.9f + juce::Random::getSystemRandom().nextFloat() * 0.1f;   // 0.9 - 1.0
    float alpha = 0.6f + juce::Random::getSystemRandom().nextFloat() * 0.4f;        // 0.6 - 1.0
    this->particleColourUser = juce::Colour::fromHSV(hue, saturation, brightness, alpha);
}

NoteLayer::~NoteLayer()
{
    //keyBoardUI.midiHandler.removeListener(this);
    openGLContext.detach();
}

void NoteLayer::paint(juce::Graphics& g)
{
    g.reduceClipRegion(getLocalBounds());

    // "Still held" cue: a soft amber glow that breathes along the leading (top) edge of every held
    // note, so a note that has grown to full height reads as alive rather than frozen/blocked. One
    // shared phase makes a held chord pulse in unison. (paint() is driven by the 60Hz timer.)
    const float heldPulse = 0.5f + 0.5f * std::sin((float) juce::Time::getMillisecondCounterHiRes() * 0.004f);
    const juce::Colour heldGlow = juce::Colour::fromHSV(0.12f, 0.9f, 1.0f, 1.0f); // app gold/amber

    for (const auto& [midiNote, note] : activeNotes)
    {
        auto bounds = note.bounds.toFloat();


        float cornerRadius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.25f;

        // Elegant frosted glass gradient using whitesmoke/neutral colors
        juce::ColourGradient fillGradient(juce::Colours::white.withAlpha(0.4f), bounds.getTopLeft(),
                                          juce::Colours::white.withAlpha(0.05f), bounds.getBottomLeft(), false);
        g.setGradientFill(fillGradient);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Clean whitesmoke outline
        g.setColour(juce::Colours::whitesmoke.withAlpha(0.9f));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.5f);

        // Pulsing amber glow on the leading edge (soft band fading downward + a crisp top line).
        const float glowHeight = std::min(bounds.getHeight(), 16.0f);
        juce::ColourGradient glowGrad(heldGlow.withAlpha(0.18f + 0.55f * heldPulse), bounds.getX(), bounds.getY(),
                                      heldGlow.withAlpha(0.0f), bounds.getX(), bounds.getY() + glowHeight, false);
        g.setGradientFill(glowGrad);
        g.fillRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), glowHeight, cornerRadius);

        g.setColour(heldGlow.withAlpha(0.35f + 0.55f * heldPulse));
        g.fillRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), 2.5f, 1.25f);
    }
    
    for (const auto& note : fallingNotes)
    {
        auto bounds = note.bounds.toFloat();
        float cornerRadius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.25f;

        juce::ColourGradient fillGradient(juce::Colours::white.withAlpha(0.4f * note.alpha), bounds.getTopLeft(),
                                          juce::Colours::white.withAlpha(0.05f * note.alpha), bounds.getBottomLeft(), false);
        g.setGradientFill(fillGradient);
        g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(juce::Colours::whitesmoke.withAlpha(0.9f * note.alpha));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.5f);
    }
}

void NoteLayer::noteOnReceived(int midiNote)
{
    juce::MessageManager::callAsync([this, midiNote]
        {
            if (activeNotes.find(midiNote) == activeNotes.end())
            {
                if (!this->isVisible() && keyBoardUI.isVisible())
                    this->setVisible(true);
                AnimatedNote newNote;
                newNote.bounds = keyBoardUI.keys[midiNote].bounds;

                newNote.height = 10.0f;
                newNote.yPosition = static_cast<float>(getHeight()) - newNote.height;

                newNote.bounds.setHeight(static_cast<int>(round(newNote.height)));
                newNote.bounds.setY(static_cast<int>(round(newNote.yPosition)));

                activeNotes[midiNote] = newNote;
                
                spawnParticlesForNote(midiNote);

                if(!isTimerRunning())
                    startTimerHz(60);
            }
        });
}

void NoteLayer::noteOffReceived(int midiNote)
{
    juce::MessageManager::callAsync([this, midiNote]
        {
            auto it = activeNotes.find(midiNote);
            if (it != activeNotes.end())
            {
                it->second.isFalling = true;
                it->second.alpha = 1.0f;
                it->second.initialHeight = it->second.height;

                //DBG("Initial height"<<it->second.initialHeight);
                fallingNotes.push_back(it->second);
                activeNotes.erase(it);
            }
        });
}

void NoteLayer::newOpenGLContextCreated()
{
    std::pair<const char*, const char*> p = getShaderChoice(currentStyle);
    const char* vertexSource = p.first;
    const char* fragmentSource = p.second;

    shader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
    if (!shader->addVertexShader(vertexSource) || !shader->addFragmentShader(fragmentSource) || !shader->link())
    {
        DBG("Shader failed to compile or link:");
        DBG(shader->getLastError());
        jassertfalse;
        shader.reset();
        return;
    }
    positionAttr = new juce::OpenGLShaderProgram::Attribute(*shader, "position");
    pointSizeAttr = new juce::OpenGLShaderProgram::Attribute(*shader, "pointSize");
    colourAttr = new juce::OpenGLShaderProgram::Attribute(*shader, "colour");

    if (openGLContext.isActive())
    {
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_POINT_SPRITE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, getWidth(), getHeight());
    }
    else {
        openGLContext.makeActive();
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_POINT_SPRITE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, getWidth(), getHeight());
    }
    openGLContext.extensions.glGenBuffers(1, &particleVBO);

#if JUCE_DEBUG
    // --- TEMPORARY DIAGNOSTIC (remove once the intermittent GL error is identified) ---
    // Install our own GL debug callback for THIS context, replacing JUCE's assert-only one, so the
    // real GL_DEBUG_TYPE_ERROR is written to a file we can read later instead of only appearing at a
    // debugger break we keep missing. Runs on the render thread (single thread per context), so the
    // static dedup state needs no locking. Skips the harmless NVIDIA "will use VIDEO memory" noise.
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

void NoteLayer::renderOpenGL()
{
    if (needsShaderRecompile.exchange(false))
    {
        openGLContextClosing();
        newOpenGLContextCreated();
    }

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (shader != nullptr && particleVBO != 0 && !particles.empty())
    {
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

        openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(Vertex)* renderVerts.size()), renderVerts.data(), GL_DYNAMIC_DRAW);

        if (positionAttr != nullptr)
        {

            openGLContext.extensions.glVertexAttribPointer(positionAttr->attributeID, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, x));
            openGLContext.extensions.glEnableVertexAttribArray(positionAttr->attributeID);
        }
        if (pointSizeAttr != nullptr)
        {
            openGLContext.extensions.glVertexAttribPointer(pointSizeAttr->attributeID, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, size));
            openGLContext.extensions.glEnableVertexAttribArray(pointSizeAttr->attributeID);
        }
        if (colourAttr != nullptr)
        {
            openGLContext.extensions.glVertexAttribPointer(colourAttr->attributeID, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, r));
            openGLContext.extensions.glEnableVertexAttribArray(colourAttr->attributeID);
        }

        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_POINT_SPRITE);
        glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive blend
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDrawArrays(GL_POINTS, 0, (GLsizei)renderVerts.size());

        // Cleanup
        if (positionAttr) openGLContext.extensions.glDisableVertexAttribArray(positionAttr->attributeID);
        if (pointSizeAttr) openGLContext.extensions.glDisableVertexAttribArray(pointSizeAttr->attributeID);
        if (colourAttr) openGLContext.extensions.glDisableVertexAttribArray(colourAttr->attributeID);
    }
}

void NoteLayer::openGLContextClosing()
{
    if (particleVBO)
    {
        openGLContext.extensions.glDeleteBuffers(1, &particleVBO);
        particleVBO = 0;
    }
    delete positionAttr;
    positionAttr = nullptr;

    delete pointSizeAttr;
    pointSizeAttr = nullptr;

    delete colourAttr;
    colourAttr = nullptr;

    shader.reset();
}

void NoteLayer::resetState()
{
    this->activeNotes.clear();
    this->fallingNotes.clear();
    this->particles.clear();
}

void NoteLayer::resetStateActiveNotes()
{
    for (auto& v : activeNotes)
    {
        int note = v.first;
        noteOffReceived(note);
    }
}

void NoteLayer::setColourParticle(juce::Colour& colour)
{
    this->particleColourUser = colour;
}

void NoteLayer::setSpawnParticleState(bool state)
{
    this->spawnParticleState = state;
}

void NoteLayer::setParticleStyle(int styleId)
{
    if (currentStyle == styleId) return;
    currentStyle = styleId;
    needsShaderRecompile = true;
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

void NoteLayer::updateParticles()
{
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < (int)particles.size(); )
    {
        auto& p = particles[i];
        if (currentStyle == 1) { // Sparks
            p.velocity.y -= 2.0f * dt; // Gravity pulls them down slightly
            p.pos += p.velocity * dt;
            p.life -= dt * 1.5f; // Fast fade
        }
        else if (currentStyle == 2) { // Dust
            p.velocity.x += (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.5f * dt; // Brownian
            p.velocity.x *= 0.95f; // Drag
            p.pos += p.velocity * dt;
            p.life -= dt * 0.6f; // Slow fade
        }
        else if (currentStyle == 3) { // Smoke
            p.velocity.x += (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.2f * dt; // Slight wiggle
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

void NoteLayer::spawnParticlesForNote(int midiNote)
{
    if (!spawnParticleState || particles.size() > 1800)
        return; // Prevent excessive spawning if many notes are played simultaneously

    auto keyBounds = this->keyBoardUI.keys[midiNote].bounds;

    float x_ndc = (keyBounds.getCentreX() / (float)getWidth()) * 2.0f - 1.0f;
    float y_ndc = -(1.0f - (keyBounds.getY() / (float)getHeight()) * 2.0f);
    y_ndc += 0.05f;
    juce::Point<float> pos(x_ndc, y_ndc);

    int count = 0;
    if (currentStyle == 1) count = 25; // Sparks
    else if (currentStyle == 2) count = 40; // Dust
    else if (currentStyle == 3) count = 15; // Smoke

    for (int i = 0; i < count; i++)
    {
        Particle p;
        if (currentStyle == 1) { // Sparks
            p.pos = pos;
            p.velocity = { (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.8f, juce::Random::getSystemRandom().nextFloat() * 1.5f };
            p.size = 2.0f + juce::Random::getSystemRandom().nextFloat() * 6.0f;
            p.life = 0.8f + juce::Random::getSystemRandom().nextFloat() * 0.4f;
            p.colour = this->particleColourUser;
        } 
        else if (currentStyle == 2) { // Dust
            float spreadX = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.08f;
            float spreadY = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.02f;
            p.pos = pos + juce::Point<float>(spreadX, spreadY);
            p.velocity = { (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.2f, juce::Random::getSystemRandom().nextFloat() * 0.4f + 0.1f };
            p.size = 3.0f + juce::Random::getSystemRandom().nextFloat() * 15.0f;
            float alpha = 0.4f + juce::Random::getSystemRandom().nextFloat() * 0.6f;
            p.colour = this->particleColourUser.withAlpha(alpha);
            p.life = 0.8f + juce::Random::getSystemRandom().nextFloat() * 0.4f;
        }
        else if (currentStyle == 3) { // Smoke
            float spreadX = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.12f;
            p.pos = pos + juce::Point<float>(spreadX, 0.0f);
            p.velocity = { (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.1f, juce::Random::getSystemRandom().nextFloat() * 0.2f + 0.05f };
            p.size = 20.0f + juce::Random::getSystemRandom().nextFloat() * 30.0f; // Very large
            float alpha = 0.15f + juce::Random::getSystemRandom().nextFloat() * 0.2f;
            p.colour = this->particleColourUser.withAlpha(alpha);
            p.life = 1.0f + juce::Random::getSystemRandom().nextFloat() * 1.0f; // longer life
        }
        particles.push_back(p);
    }
}


void NoteLayer::visibilityChanged()
{
    if (!isVisible())
        stopTimer();
    //else startTimerHz(60);
}

void NoteLayer::timerCallback()
{
    const float dt = 1.0f / 60.0f;
    const float riseSpeed = 400.0f;
    const float fadeSpeedBase = 400.0f;
    const float fadeTimeBase = 6.0f;
    const float shrinkSpeedBase = 150.0f;
    const float windowHeight = static_cast<float>(getHeight());

    const float maxNoteHeight = getHeight(); // or getHeight() if dynamic

    for (auto& pair : activeNotes)
    {
        AnimatedNote& n = pair.second;

        n.yPosition -= riseSpeed * dt;
        n.height += riseSpeed * dt;

        // Clamp growth
        if (n.height > maxNoteHeight)
        {
            float bottom = n.yPosition + n.height;
            n.height = maxNoteHeight;
            n.yPosition = bottom - n.height;
        }

        n.bounds.setY(static_cast<int>(std::round(n.yPosition)));
        n.bounds.setHeight(static_cast<int>(std::round(n.height)));
    }

    // Update falling notes (released notes)
    for (auto it = fallingNotes.begin(); it != fallingNotes.end(); )
    {
        AnimatedNote& n = *it;

        // Move upward at a constant speed, identical to active notes
        n.yPosition -= riseSpeed * dt;

        // If the top of the note hits the top of the screen (y < 0), shrink it smoothly!
        if (n.yPosition < 0.0f)
        {
            n.height += n.yPosition; // Shrink the height by the overshoot amount
            n.yPosition = 0.0f;      // Pin the top to the screen boundary
        }

        // Calculate the bottom edge (tail)
        float tailY = n.yPosition + n.height;

        // Smoothly fade out as the tail disappears
        float fadeZone = 80.0f;
        if (tailY < fadeZone)
        {
            n.alpha = std::max(0.0f, tailY / fadeZone);
        }

        // Apply position to bounds
        int yInt = static_cast<int>(std::round(n.yPosition));
        int hInt = std::max(1, static_cast<int>(std::round(n.height)));
        n.bounds.setY(yInt);
        n.bounds.setHeight(hInt);

        // Erase note once the tail is off-screen or height is fully shrunk
        if (tailY <= 0.0f || n.height <= 0.1f || n.alpha <= 0.0f)
        {
            it = fallingNotes.erase(it);
        }
        else
        {
            ++it;
        }
    }


    updateParticles();

    if (!particles.empty() || !activeNotes.empty() || !fallingNotes.empty())
    {
        openGLContext.triggerRepaint();
        repaint();
    }
    else
    {
        particles.clear();
        fallingNotes.clear();
        activeNotes.clear();
        openGLContext.triggerRepaint();
        repaint();

        stopTimer();
    }
}

void NoteLayer::resized()
{
    // Do NOT touch OpenGL here: resized() runs on the message thread, while the context is owned by
    // the dedicated GL render thread. Calling makeActive()/glViewport() here makes the context current
    // on the wrong thread and races the render thread -> intermittent GL_DEBUG_TYPE_ERROR (the crash).
    // JUCE already sets the correct viewport every frame before renderOpenGL() (see OpenGLContext
    // CachedImage::renderFrame), so nothing GL-related is needed on resize -- just ask for a redraw.
    openGLContext.triggerRepaint();
}