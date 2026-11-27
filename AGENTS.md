# Codex project notes

## Start here

This repository is the C port of Blitz Basic 2 for AmigaOS (68000): compiler `blitz2`, editor `ted`, and the `acidlibs`/`deflibs` libraries. The repository sources are authoritative; original sources are in `archive/`, and translation notes are in `docs/`.

Before project work, read `CLAUDE.md` for the full project conventions and constraints. It remains the detailed project guide and source of truth; apply its project rules in Codex sessions too. Then read only the relevant project documentation (`README.md`, `docs/development.md`, `docs/structure.md`, `tests/README.md`, relevant OpenSpec files) for the task.

The current coordinator handoff is `~/.local/share/bb2-c-work/decisions/handoff.md`. It is session state, not a permanent rule: verify its claims against `git log`, `git worktree list`, and the relevant GitHub state before acting. The scratchpad `~/.local/share/bb2-c-work/` contains reports, logs, experiments, and drafts; it is large, so locate and read only material relevant to the task. Do not put working data in its `decisions/` directory.

## Decisions

User decisions and their scope are recorded in `~/.local/share/bb2-c-work/decisions/`. Read `decisions/README.md` for the layout and `decisions/decisions.md` first. For a question that is not settled by an applicable active decision, read `process.md` and `catalog.md` completely, then find relevant entries in `areas/`, `tasks/`, `rounds/`, or `archive/` as the README describes. Preserve the user's wording and scope; distinguish active decisions from proposals, and report contradictions instead of silently resolving them. Do not edit these files unless the user specifically asks.

## Claude agent definitions

`.claude/agents/` contains Claude Code role prompts: `analyst`, `planning`, `implementer`, `tester`, `docs`, and `site-writer`. Their role guidance and workflows are useful, but the files are not loaded as Codex agents: Claude model names, tool lists, and Claude-specific commands need translation. Codex supports project agents as standalone TOML files under `.codex/agents/`; a role can set its own `model` and `model_reasoning_effort`. See the official [Codex subagents guide](https://developers.openai.com/codex/concepts/multi-agents) before converting a role. Use Codex subagents only when the user or applicable instructions explicitly request delegation.

## Reminders

- Check `git status` before editing; stage only files for the requested topic. Keep commits focused and do not push unless explicitly asked.
- Repository and GitHub text is English. Keep original files under `archive/` unchanged.
- Follow the original-bug workflow and result-comparison rules in `CLAUDE.md` and `docs/development.md` for compiler/library behavior changes.
- Build and test commands, emulator constraints, licensing, and detailed code conventions are in `CLAUDE.md`, `README.md`, and `tests/README.md`; consult them when applicable.
