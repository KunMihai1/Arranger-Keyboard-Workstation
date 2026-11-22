#pragma once

#include "NoteLayer.h"

// Names shared by NoteLayer.cpp and NoteLayerParticles.cpp (one class split across two files, ADR-003).
namespace NoteLayerNames
{
    namespace ShaderNames
    {
        inline const char* const position       = "position";
        inline const char* const local          = "local";
        inline const char* const rect           = "rect";
        inline const char* const viewport       = "viewport";
        inline const char* const glowRgb        = "glowRgb";
        inline const char* const antialiasWidth = "aa";
        inline const char* const pointSize      = "pointSize";
        inline const char* const colour         = "colour";
        inline const char* const time           = "uTime";
    }

    namespace Components
    {
        inline constexpr int xy = 2;
        inline constexpr int size = 1;
        inline constexpr int rect = 4;
        inline constexpr int rgba = 4;
    }

    namespace Timing
    {
        inline constexpr double secondsPerMillisecond      = 0.001;
        inline constexpr float  secondsPerMillisecondFloat = 0.001f;
        inline constexpr double noFrameYet                 = 0.0;
        inline constexpr double longestStepSeconds         = 0.05;   // after a stall, step at most this far
        inline constexpr float  noStep                     = 0.0f;
        inline constexpr int    firstUniformId             = 0;      // negative: the shader has no such uniform
    }

    inline constexpr GLuint  noBuffer    = 0;
    inline constexpr GLsizei oneBuffer   = 1;
    inline constexpr GLint   firstVertex = 0;
    inline constexpr float   emptySize   = 0.0f;
}
