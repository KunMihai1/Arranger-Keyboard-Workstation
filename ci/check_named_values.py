"""House rule check: no bare numbers and no inline strings in C++ code - every value has a name.

A literal is allowed only where a name is being given to it:
  - a constant definition:   constexpr int keyCount = 88;   const juce::String title = "Start";
    (including a brace initializer that continues over several lines)
  - an enumerator value:      enum class Mode { off = 0, on = 1 };
  - #include / #pragma / #define / #error lines, static_assert, and extern "C"
Comments are ignored. `true`, `false` and `nullptr` are not literals for this rule.

Files not converted yet are listed in ci/named-values-allowlist.txt; the list may only shrink. A file on
the list that has become clean must be removed from it (so it cannot regress unnoticed).

Usage: python ci/check_named_values.py [--list-violations FILE]   (exit code 1 on any problem)
"""
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOTS = ("Source", "tests", "ci")
SKIP_DIRS = ("JuceLibraryCode", "Builds")
ALLOWLIST = os.path.join(REPO, "ci", "named-values-allowlist.txt")

STRING = re.compile(r'(?:u8|u|U|L)?R?"(?:[^"\\\n]|\\.)*"')
CHAR = re.compile(r"'(?:[^'\\\n]|\\.)+'")
NUMBER = re.compile(r"(?<![\w.])(?:0[xX][0-9a-fA-F']+|\d[\d']*\.?\d*(?:[eE][+-]?\d+)?|\.\d+(?:[eE][+-]?\d+)?)[uUlLfF]*(?![\w.])")
CONSTANT_DEFINITION = re.compile(
    r"^\s*(?:static\s+|inline\s+|extern\s+)*(?:constexpr|const)\b[^;(]*?\b\w+\s*(?:\[[^\]]*\]\s*)?(?:=|\{)")
ENUMERATOR = re.compile(r"^\s*\w+\s*=\s*[^;]*,?\s*$")
DIRECTIVE = re.compile(r"^\s*(?:#\s*(?:include|pragma|define|error|warning|if|elif)|static_assert|extern\s+\"C\")")


def strip_comments(text):
    """Blank out comments but keep line structure (and string contents) intact."""
    out, i, n = [], 0, len(text)
    while i < n:
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i))
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r"[^\n]", " ", text[i:j]))
            i = j
        elif text[i] == '"':
            m = STRING.match(text, i)
            j = m.end() if m else i + 1
            out.append(text[i:j])
            i = j
        elif text[i] == "'":
            m = CHAR.match(text, i)
            j = m.end() if m else i + 1
            out.append(text[i:j])
            i = j
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def violations(path):
    text = strip_comments(open(path, encoding="utf-8", errors="replace").read())
    found = []
    inside_constant_braces = 0
    in_enum = False
    for number, line in enumerate(text.split("\n"), start=1):
        stripped = line.strip()
        if re.match(r"^\s*enum\b", line):
            in_enum = True
        allowed = (inside_constant_braces > 0 or bool(CONSTANT_DEFINITION.match(line))
                   or bool(DIRECTIVE.match(line)) or (in_enum and bool(ENUMERATOR.match(line))))
        if CONSTANT_DEFINITION.match(line) or inside_constant_braces > 0:
            inside_constant_braces = max(0, inside_constant_braces + line.count("{") - line.count("}"))
        if in_enum and "}" in line:
            in_enum = False
        if allowed or not stripped:
            continue
        without_strings = CHAR.sub("''", STRING.sub('""', line))
        literals = [m for m in STRING.findall(line) if m != '""']
        literals += CHAR.findall(line)
        literals += NUMBER.findall(without_strings)
        if literals:
            found.append((number, stripped, literals))
    return found


def project_files():
    for root in ROOTS:
        for d, _, names in os.walk(os.path.join(REPO, root)):
            if any(skip in d for skip in SKIP_DIRS):
                continue
            for name in sorted(names):
                if name.endswith((".cpp", ".h")):
                    yield os.path.relpath(os.path.join(d, name), REPO).replace(os.sep, "/")


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "--list-violations":
        for number, line, literals in violations(os.path.join(REPO, sys.argv[2])):
            print(f"{sys.argv[2]}:{number}: {', '.join(literals)}    {line}")
        return 0

    allowlisted = set()
    if os.path.exists(ALLOWLIST):
        for entry in open(ALLOWLIST, encoding="utf-8"):
            entry = entry.strip()
            if entry and not entry.startswith("#"):
                allowlisted.add(entry)

    problems = []
    for relative in project_files():
        found = violations(os.path.join(REPO, relative))
        if found and relative not in allowlisted:
            first = found[0]
            problems.append(f"{relative}:{first[0]}: unnamed value(s) {', '.join(first[2])} - "
                            f"{len(found)} line(s); give each value a name (AGENTS.md, 'Names, not values')")
        elif not found and relative in allowlisted:
            problems.append(f"{relative}: is clean now - remove it from ci/named-values-allowlist.txt")
    for missing in sorted(allowlisted - set(project_files())):
        problems.append(f"{missing}: listed in ci/named-values-allowlist.txt but does not exist - remove it")

    for problem in problems:
        print(problem)
    print(f"named-values check: {len(problems)} problem(s); {len(allowlisted)} file(s) still on the allowlist")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
