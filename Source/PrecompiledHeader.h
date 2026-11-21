/*
    Precompiled header for the app and the test runner (enabled in both .jucer files).
    Every non-module source file gets it force-included, so JUCE is parsed once per build
    instead of once per file. Measured 2026-11-21: one-file rebuild 8.2 s -> 2.1 s,
    clean test build on 4 cores 202 s -> 54 s. Keep it to stable, widely used headers only.
    Projucer COPIES this file into the build folder on save (JucePrecompiledHeader_<config>.h),
    so an edit here only takes effect after re-saving both .jucer files - then everything rebuilds.
*/
#pragma once

#include <JuceHeader.h>
