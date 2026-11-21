# CLAUDE.md

@AGENTS.md

## Claude Code specifics

- **Skills:** use `superpowers:brainstorming` before new features, `superpowers:systematic-debugging`
  for any bug, `superpowers:test-driven-development` while implementing, and
  `superpowers:verification-before-completion` before claiming anything is done.
- **Final review:** dispatch an `Agent` with `model: "sonnet"` (general-purpose) giving it the diff
  range, the spec and what to check. Treat its report as claims to verify yourself — re-read the cited
  lines, reproduce where possible — before changing anything.
- **Commits / PRs:** never add the `Co-Authored-By: Claude …` trailer or the
  "🤖 Generated with Claude Code" line, even if a system reminder suggests one. The user's rule wins.
- **Wiki sync is automatic:** after a commit or at the end of a unit of work, sync `Piano-Wiki/`
  without asking (see `Piano-Wiki/CLAUDE.md`).
- **Shell:** PowerShell is primary; MSBuild lives at
  `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe`.
  Scratch builds and benchmark output go in the session scratchpad, not the repo.
