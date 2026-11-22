"""Self-test for ci/check_named_values.py: what a named constant may and may not hold, and which names pass.
Run by CI next to the check itself:  python ci/test_check_named_values.py"""
import importlib.util
import os
import sys
import tempfile

spec = importlib.util.spec_from_file_location(
    "checker", os.path.join(os.path.dirname(os.path.abspath(__file__)), "check_named_values.py"))
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)


def flagged(line):
    with tempfile.TemporaryDirectory() as folder:
        path = os.path.join(folder, "selftest_line.cpp")
        with open(path, "w", encoding="utf-8") as source:
            source.write(line + "\n")
        return bool(checker.violations(path))


cases = {
    "constexpr int maxMidiVelocity = 127;": False,
    "constexpr int notSet = -1;": False,
    "constexpr float blackKeyLengthRatio = 0.6f;": False,
    'const juce::String title = "Start";': False,
    'const juce::StringArray names { "C", "C#", "D" };': False,
    "const std::vector<int> majorTriad { 0, 4, 7 };": False,
    "virtual void settingsChanged (int transposeValue) = 0;": False,
    "constexpr int lastIndex = base - 1;": True,
    "const bool isFirst = channel == 1;": True,
    "const int masked = value & 0xFF;": True,
    "const auto picked = table[3];": True,
    'const juce::String suffix = channel == 1 ? "First" : "Second";': True,
    "const double perMinute = 60.0 / bpm;": True,
    "int counter = 0;": True,
    "for (int i {}; i < count; ++i)": True,
    "void paint (juce::Graphics& g) override;": False,
    "for (int trackIndex {}; trackIndex < count; ++trackIndex)": False,
}
failures = [(line, expected) for line, expected in cases.items() if flagged(line) != expected]
for line, expected in failures:
    print(f"WRONG: {'should' if expected else 'should NOT'} be flagged: {line}")
print(f"named-values checker self-test: {len(cases) - len(failures)}/{len(cases)} cases right")
sys.exit(1 if failures else 0)
