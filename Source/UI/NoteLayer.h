/*
  ==============================================================================

    NoteLayer.h
    Created: 20 May 2025 1:36:54pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include <vector>
#include <utility>
#include <memory>
#include <atomic>
#include <mutex>
#include <algorithm>

#include "MidiHandler.h"
#include "KeyboardUI.h"
#include "NoteField.h"

/**
    Displays active and falling MIDI notes with particle effects.

    Everything is drawn on the OpenGL render thread (notes with a rounded-rect SDF shader,
    particles as point sprites) and JUCE component painting is disabled. The old juce::Graphics
    path made JUCE hold the message-thread lock for the whole paint every frame: measured 18 fps
    and the UI blocked for up to ~90 ms with ~90 notes in a Debug build. See
    docs/superpowers/specs/2026-11-21-note-rendering-perf-design.md.

    Threading: the public methods are called on the message thread. They post commands to a
    small mutex-guarded queue; renderOpenGL() drains it and owns all animation state.
*/

/** Particle looks of the note layer (the particle-style setting). */
namespace ParticleStyles
{
    constexpr int sparks = 1;
    constexpr int dust   = 2;
    constexpr int smoke  = 3;

    /** A style read from the settings file: anything but Sparks / Dust / Smoke reads as Dust (the default). */
    constexpr int fromStored (int stored) { return stored >= sparks && stored <= smoke ? stored : dust; }
}

class NoteLayer : public juce::Component, public MidiHandlerListener, private juce::OpenGLRenderer
{
public:
    /**
     * @brief Constructor
     * @param referenceKeyboard Reference to the KeyboardUI to sync note positions.
     */
    NoteLayer(KeyboardUI& referenceKeyboard);

    /** Destructor */
    ~NoteLayer() override;

    /** @brief Called when the component is resized */
    void resized() override;

    /** @brief Called when a MIDI note-on is received (any thread) */
    void noteOnReceived(int midiNote) override;

    /** @brief Called when a MIDI note-off is received (any thread) */
    void noteOffReceived(int midiNote) override;

    /** @brief Called when OpenGL context is created */
    void newOpenGLContextCreated() override;

    /** @brief OpenGL rendering callback */
    void renderOpenGL() override;

    /** @brief Called before OpenGL context is destroyed */
    void openGLContextClosing() override;

    /** @brief Resets all visual states including active and falling notes */
    void resetState();

    /** @brief Releases every active note (they fall away as on note-off) */
    void resetStateActiveNotes();

    /** @brief Sets the particle color for note effects */
    void setColourParticle(juce::Colour& colour);

    /** @brief Enables or disables spawning of particles */
    void setSpawnParticleState(bool state);

    /** @brief Sets the particle skin style (1 = Sparks, 2 = Dust, 3 = Smoke) */
    void setParticleStyle(int styleId);

    /** @brief A page covers the note area: stay hidden even when a note arrives (otherwise the layer shows
        itself on a note while the keyboard is visible, and the GL layer would cover the page). */
    void setHeldHidden(bool shouldStayHidden);

    std::pair<const char*, const char*> sparksShader();
    std::pair<const char*, const char*> dustShader();
    std::pair<const char*, const char*> smokeShader();
    std::pair<const char*, const char*> getShaderChoice(int styleId);

private:
    static constexpr float fullLife            = 1.0f;
    static constexpr float defaultParticleSize = 6.0f;

    struct Command
    {
        enum class Type { noteOn, noteOff, releaseAll, clear };
        Type type;
        int midiNote {};
        juce::Rectangle<int> keyBounds;
    };

    /** Message thread: queue a command for the render thread and wake it. */
    void post(Command command);

    /** Message thread: what noteOnReceived used to do inline (needs KeyboardUI). */
    void handleNoteOn(int midiNote);

    // --- render thread only ---------------------------------------------------------------
    void applyCommands(float width, float height);
    void updateParticles(float dt);
    void spawnParticlesForNote(juce::Rectangle<int> keyBounds, float width, float height);
    void createParticleShader();
    void releaseParticleShader();
    void createNoteShader();
    void drawNotes(float width, float height);
    void drawParticles();

    /** @struct Particle
     *  @brief Represents a particle in the OpenGL particle system.
     */
    struct Particle {
        juce::Point<float> pos;     /**< Particle position in NDC coordinates */
        juce::Point<float> velocity;/**< Particle velocity */
        float life = fullLife;      /**< Remaining life (0 = dead) */
        float size = defaultParticleSize; /**< Particle size */
        juce::Colour colour;        /**< Particle color */
    };

    struct Vertex {
        float posX, posY;
        float size;
        float red, green, blue, alpha;
    };

    struct NoteVertex {
        float px, py;          // position, NoteLayer-local logical px
        float lx, ly;          // position inside the note's rectangle
        float rectWidth, rectHeight, alpha, pulse;  // rectangle size, fade, held-glow phase (-1 = released)
    };

    KeyboardUI& keyBoardUI;                    /**< Reference to keyboard UI */
    /** Made once on the message thread: the first SafePointer to a component lazily creates its
        shared weak-reference holder, which must not race. Copies (from the MIDI thread) are safe. */
    const juce::Component::SafePointer<NoteLayer> safeThis { this };
    juce::OpenGLContext openGLContext;         /**< OpenGL rendering context */

    // Shared between the message thread and the render thread, guarded by commandLock.
    std::mutex commandLock;
    std::vector<Command> commands;
    float layerWidth {}, layerHeight {};
    juce::Colour particleColourUser;           /**< Particle color */
    bool spawnParticleState = false;           /**< Whether particle spawning is enabled */
    bool heldHidden = false;                   /**< A page covers the note area (setHeldHidden) */
    int currentStyle = ParticleStyles::dust;   /**< ParticleStyles */
    bool styleChanged = false;

    // Render thread only.
    std::vector<Command> pendingCommands;
    NoteField field;
    std::vector<Particle> particles;           /**< Active particles */
    juce::Random rng;                          /**< Particle randomness (getSystemRandom() is shared, not thread-safe) */
    juce::Colour renderParticleColour;
    bool renderSpawnParticles = false;
    int renderStyle = ParticleStyles::dust;
    double lastFrameTime {};
    bool animating = false;

    std::unique_ptr<juce::OpenGLShaderProgram> shader; /**< Shader program for particles */
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> positionAttr, pointSizeAttr, colourAttr;
    GLuint particleVBO {};                     /**< Vertex buffer object for particles */
    std::vector<Vertex> renderVerts;           /**< Preallocated vertex buffer */

    std::unique_ptr<juce::OpenGLShaderProgram> noteShader;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> notePositionAttr, noteLocalAttr, noteRectAttr;
    GLuint noteVBO {};
    std::vector<NoteVertex> noteVerts;

    static constexpr size_t maxQueuedCommands = 4096; // bound the queue while the layer is hidden
};
