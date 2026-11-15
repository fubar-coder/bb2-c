## Context

See proposal.md - Why. State today: the detail pages are Markdown with
headed sections (`### Title {#id}`) for bugs ordinary programs meet and
bulleted lists under `###` headings (no per-entry anchor) for the rest;
each entry links its issue as `[#N](https://github.com/fubar-coder/bb2-c/issues/N)`.
`libraries` and `editor` are page bundles (`index.md`). The site is Hugo
with Hextra, built by `.github/workflows/pages.yml`; there are shortcodes
(`site/layouts/_shortcodes/library-list.html`), partials, and data under
`site/data/`. The Hugo version is 0.154.5 (`site/README.md`), run through
podman, not installed on the host.

## Goals / Non-Goals

**Goals:** a reader finds the incompatibilities first; rows cannot drift
from the detail pages; no new tool outside Hugo.

**Non-Goals:** no change of the detail texts; no listing of DIFF cases,
port bugs or won't-fix items; no search beyond filter and sort (Hextra's
search finds the page already).

## Decisions

1. **Data file `site/data/fixoverview.yaml`.** One YAML list, entries
   `{issue, severity, topic, library, command, text, page, anchor}`;
   `page` is the fixed-bugs page (`libraries`, `compiler`, `editor`,
   `running-blitz2`), `anchor` a heading id on it. YAML as the other data
   (`data/cmddesc`). One file, because rows of all rounds are sorted
   together and the diff of a fix is one entry. Alternative: front matter
   of each detail page - rejected, the bulleted entries have no page of
   their own and the table needs one list. Block scalars or quotes for
   texts (README rule on `": "`).
2. **Anchors.** A bug with its own section links to its `{#id}`. A bullet
   entry links to the id of its `###` group heading (Hugo generates ids
   for them: `#files-and-banks`); no per-bullet ids are added, so the
   detail pages do not change. Where a group heading id would be
   ambiguous the heading gets an explicit `{#id}` (only that edit to the
   detail pages).
3. **Rendering.** Shortcode `fix-overview` loops the data, sorted in Hugo
   by severity, topic, issue (the order without JavaScript); renders a
   `<table>` with `<th scope="col">` headers; links built by `relref`
   with page and anchor (`relref . (printf "%s#%s" ...)`), issue links
   from the number. Inside a severity level the rows are ordered by issue number; the
   severity-1 rows also stand in a warning box above the table.
4. **JavaScript.** One file `site/assets/js/fix-overview.js`, loaded by
   the shortcode only on that page through `resources.Get | minify |
   fingerprint` (Hugo Pipes, built in; no external tool). Behaviour:
   header text is wrapped into a `<button>` by the script (without
   JavaScript no dead buttons exist), click toggles ascending/descending,
   sets `aria-sort` on the `th`; two `<select>` filters (severity, topic)
   with `<label>` and an `aria-live="polite"` count; rows carry
   `data-severity` and `data-topic`, hidden by the `hidden` attribute.
   Sorting is stable and uses the data attributes (numbers numerically,
   text with `localeCompare`). The page's own style is a short block in
   the shortcode or a CSS asset, using Hextra's classes where they exist
   (the table classes of the theme, dark mode through its variables).
5. **Severity text.** The four levels are defined once in the data file's
   header section (`levels:` list) and shown above the table and in the
   filter, so the page and the filter cannot disagree.
6. **The check in Hugo.** Feasible for the first two classes, in a
   partial called by the shortcode (so it runs on every build that renders
   the page, i.e. every build):
   - anchor exists: `site.GetPage` of the page path, then
     `.Fragments.Identifiers.Contains` on it; `errorf` with the issue.
     Hugo parses headings with explicit ids into the fragments, so this
     checks real headings, not text.
   - severity in 1-4, topic in the `topics` list of the data file, no
     duplicate issue, required fields present: plain template logic with
     `errorf`.
   - every fixed-bugs issue has a row: `.RawContent` of each detail page
     matched with `findRE` for `issues/(\d+)`; an issue that is neither a
     row nor on the data file's `notRows` list (issue, reason) fails the
     build. Cross-references to issues that are not fixes (kept bugs,
     other topics) go on `notRows`; that is the cost of a raw text scan,
     accepted because it is the only check that catches a forgotten row.
   `errorf` fails the build; `warnf` is not used, since `site/README.md`
   requires a build without warnings.
   Limits: the check proves the heading exists, not that the entry is
   below it; a raw-text match may hit an issue in a code block (it would
   have to go on `notRows`, visible in review).
7. **Test of the check.** Hugo is only available via podman, so no
   host-ctest dependency. The check takes its data file name from a site
   parameter (`params.fixOverviewData`, default `fixoverview`); a CMake
   test (label `site`, skipped with 77 if podman or the image is missing,
   like the emulator tests) builds the site into a temporary directory
   with `--environment`-selected broken data files under
   `site/tests/` (missing anchor, severity 5, unknown topic, duplicate
   issue, entry without row) and expects each to fail with its message
   and the real file to build. This keeps the test files out of
   `site/data/`. Alternative: only run the check in the Pages workflow
   (no negative test) - rejected, CLAUDE.md demands one for every check.
   Alternative: a C tool in `tools/` that parses the Markdown - rejected,
   duplicates Hugo's heading ids and is the heavier path; the rule allows
   "Hugo/CMake only" and Hugo knows the real ids.
8. **Initial classification.** A one-time job by an agent over the 299
   linked issues: read the entry, the issue and the fix commit, propose
   severity, topic, library/command and the one sentence; the agent writes
   the YAML proposal into `~/.local/share/bb2-c-work/`, not into the
   repository. The severity 1 rows (expected: a handful) go to the user
   for confirmation before the data file is committed. Severity is
   judged by one question per row: "can a program that ran correctly with
   the original behave differently now?" - yes: 1; if only a refused
   source: 2; only with runtime error checking: 3; else 4. The sentence is
   written for the reader (what changes), not copied from the detail page.
9. **Workflow.** The coordinator's website step (CLAUDE.md fix step 3,
   `testing/bug-fix-workflow`) adds the row with the entry; the
   implementer's report proposes severity and topic. The consistency
   check makes the step enforceable. CLAUDE.md is changed only with the
   user's OK.

## Risks / Trade-offs

- [Wrong severity, especially a missed severity 1] -> level 1 rows are
  confirmed by the user; the question in Decision 8 is applied to every
  row, and the report lists all rows classified 4 whose text mentions
  "result" or "value" for a second look.
- [Table with about 250 rows is long without filter] -> filters and a
  count; the page is usable without JavaScript, sorted by severity.
- [Raw-text scan produces false positives] -> `notRows` with reasons.
- [Hugo template check is slower to write than a tool] -> it is small and
  runs inside the normal build.
- [Hextra updates change table styling] -> the page uses theme classes
  and few own rules; checked at theme updates by looking at the page.

## Migration Plan

Add data, shortcode, page and check in one change; fill the data once;
publish with the next push. Rollback: remove the page and the shortcode
call; the detail pages are unchanged.

## Decisions on the open questions (user, 2026-11-15)

1. Title "Important Fixes & Breaking Changes"; the severity-1 rows (the handful that can really break existing programs) stand in a prominent warning box above the table. File name to match (e.g. `important-fixes.md`).
2. No `rank` field: severity 1 is a small set shown in the warning box; inside a level the rows are ordered by issue number.
3. Topics (fixed list): Graphics, Strings, Numbers & dates, Files, Input, Windows & screens, Gadgets & menus, Sound, Data structures, System, Compiler, Editor.
4. One row per issue; the affected commands in the column, separated by commas.
5. Editor and compiler entries show "TED" or "Compiler" in the Library/command column.
6. The negative test of the check runs only locally (ctest, skipped without podman); the `.RawContent` scan is acceptable, no marker per entry.
7. Severity-3 rows name the runtime error in the sentence, in at most five words.
