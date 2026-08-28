# Piano-App Wiki: LLM Wiki

Mode: B (Repository / Architecture)
Purpose: Token-efficient, persistent knowledge base for the Piano-app JUCE synth so any session reads a small hot cache + index instead of re-crawling the codebase.
Owner: KunMihai
Created: 2026-06-06
Tracked: yes - committed to the Piano-app repo as of 2026-08-29 (was local-only/gitignored before that). Per-machine churn (.obsidian/workspace.json, .vault-meta/pending-changes.log) stays ignored.

## Structure

```
Piano-Wiki/
├── .raw/                 # immutable source dumps — never modify
├── _templates/           # note templates (module, decision, concept, source)
├── wiki/
│   ├── index.md          # master catalog — update on every change
│   ├── log.md            # append-only, newest at top
│   ├── hot.md            # ~500-word recent-context cache
│   ├── overview.md       # executive summary
│   ├── modules/          # one page per subsystem (+ _index.md)
│   ├── components/        # reusable UI/functional components
│   ├── decisions/        # ADRs
│   ├── dependencies/      # external libs/services
│   ├── flows/            # data/signal/control flows
│   ├── concepts/         # DSP/synthesis/MIDI domain knowledge
│   └── meta/             # dashboards, lint reports
└── CLAUDE.md             # this file
```

## Conventions

- All notes use flat YAML frontmatter: type, title, created, updated, tags, status (minimum).
- Wikilinks use [[Note Name]] format: filenames are unique, no paths needed.
- .raw/ contains source documents: never modify them.
- wiki/index.md is the master catalog: update on every ingest.
- wiki/log.md is append-only: never edit past entries. New entries go at the TOP.
- Update wiki/hot.md after every ingest, significant query, and at session end. Overwrite completely, keep under 500 words.

## Operations

- Ingest: drop source in .raw/, say "ingest [filename]".
- Query: ask any question — read hot.md, then index.md, then drill in.
- Lint: say "lint the wiki" for a health check.
- Save: say "save this" to file the current conversation.

## Keeping the wiki in sync with code (Mode B: hook + on-demand)

A git `post-commit` hook (`.git/hooks/post-commit` in the Piano-app repo, local-only)
auto-appends every commit's hash, message, and changed files to
`.vault-meta/pending-changes.log`. This costs nothing — it just captures *what* changed.

When the user says **"update the wiki"** (or after finishing a feature), Claude must:

1. Read `.vault-meta/pending-changes.log` (the cheap list of what changed since last sync).
   - If empty, also diff `git log <last_synced_commit>..HEAD` as a fallback.
2. For each meaningful change, update the affected page(s) under `wiki/` —
   create/update `modules/`, `components/`, `flows/`, `decisions/`, `concepts/` as needed.
   Only read the actual source files for changes that need synthesis; don't re-crawl everything.
3. Append a dated entry to `wiki/log.md` (newest at TOP) summarizing what was synced.
4. Rewrite `wiki/hot.md` (≤500 words) with the new recent context.
5. Update `wiki/index.md` if new pages were added.
6. Advance `last_synced_commit` in `.vault-meta/last-sync.json` to current HEAD
   (`git rev-parse HEAD`) and reset `.vault-meta/pending-changes.log` back to its header.

This keeps the expensive LLM pass surgical (reads the diff, not the codebase) and on-demand,
while the hook guarantees no commit is ever missed.
