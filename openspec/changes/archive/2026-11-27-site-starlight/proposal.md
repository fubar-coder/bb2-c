# Proposal: the website on Astro Starlight

## Why

The user decided on 2026-11-27 to move the website from Hugo with the
Hextra theme to Astro Starlight ("Dann plane bitte die spec change für
Starlight.", `rounds/tree/decisions.md`, "2026-11-27 Site: migration to
Astro Starlight"), after the spike of 2026-11-26/27 showed that all five
sample parts work and estimated 30-40 h for the whole site
(`bb2-c-work/site-generator-spike/report.md`). The site and its generator
are exempt from the C/C++ rule (D-002, 2026-11-26 addition; CLAUDE.md rule
6), so the generator's templates, components and plugins may be
TypeScript/JavaScript.

## What Changes

- `site/` becomes an Astro project with Starlight: `package.json` with
  exact versions and a committed `package-lock.json`, `astro.config.mjs`,
  the site's remark/rehype plugins, components and content loader in
  `site/src/`, the pages in `site/src/content/docs/` (the Hugo URLs kept:
  `/bb2-c/docs/...`).
- The 193 Hugo pages are converted by a converter that is run first over
  all pages, with the count of the manual fixes, as an early task whose
  result may revise this plan; the converted pages are committed once at
  the cut-over, the converter is removed with Hugo.
- The Hextra shortcodes become Starlight's components and asides; the
  site's shortcodes, partials and render hooks (fixed-bugs-library,
  fixed-bugs-libraries, library-list, fix-overview, the issue-link badges,
  the command reference with its 37 AmigaOS pages and their table form)
  become remark directives, an Astro component and a content loader that
  read `site/data` as today.
- The relrefs of the pages (649 in 144 files) and of `site/data/libintros`
  and `site/data/cmddesc` (46 in 39 files) become site paths
  (`/docs/...`); the base `/bb2-c/` is added by the build. Internal links
  and their anchors are checked by the build.
- The check of the overview of fixed bugs (`fix-overview/check.html`) is
  ported into the site build, with its broken-input tests
  (`site_fixoverview_*`) kept by name.
- The redirect pages of the `aliases` (130) with the anchor kept and
  `anchorMoves` are written by the build; a committed list of the Hugo
  site's URLs is checked so that every old URL keeps working.
- A Blitz Basic TextMate grammar (Shiki) with the command, object and
  AmigaOS call names from `site/data/libraries`: the ` ```blitzbasic `
  blocks get highlighting (today plain text).
- The build has no warnings (the empty i18n collection, the chunk size
  warning and any other are silenced at their cause or by configuration).
- `.github/workflows/pages.yml`: Doxygen into `site/public/api/`, the
  Astro build, upload of `site/dist/`.
- `tools/siteexamples` and `tests/site-examples/examples.txt`: the new
  content directory and file names (125 entries by path and block number;
  the example names and their expected outputs stay).
- Docs: `site/README.md`, `docs/development.md`, `docs/structure.md`, the
  site page "Building the documentation"
  (`project-structure/build-docs`), `Doxyfile`, `.gitignore`; a drafted
  wording for `.claude/agents/site-writer.md`, shown to the user before it
  is changed.
- **BREAKING** (for writers of the site, not for readers): Hugo, Hextra,
  `hugo.yaml`, `go.mod`, `layouts/`, the shortcode and relref syntax are
  removed at the cut-over. Old URLs keep working.

Not in scope: new texts or a restructuring of the site (the content stays
as it is, the site follow-ups of "2026-11-26 Site generator spike starts;
site follow-ups after it" come after this change); search tuning beyond
Starlight's Pagefind; a visible "last updated" date; a lazy-loading
rework of Mermaid beyond what astro-mermaid does; a page of the compiler
keywords in the command reference; i18n.

Results (CLAUDE.md rule 3): no executable, TED file, `acidlibs`,
`deflibs` or library file changes. The site's HTML changes (the purpose of
the change). The site examples' extracted sources change only in their
first line (the origin path); the compiled examples and their outputs stay
the same (checked with the prev build). The keyword export (Q1) adds a committed
data file (`site/data/compiler/blitz2.json`) and a check in blitz2's build
step; no result changes.

## Capabilities

### New Capabilities

- `site/site-build` (not `site/build`: the repository's `.gitignore`
  ignores every `build/` directory): how the website is built: pinned generator versions and
  lock file, a build without warnings, checked internal links, the redirect
  pages of old URLs and the check that every old URL is still served, no
  leftover syntax of the former generator, the Blitz Basic highlighting
  from the library data.

### Modified Capabilities

- `site/fix-overview`: "Links keep the base path" (no relref; site paths
  with the base added by the build) and "The old URLs still work" (the
  build writes the redirect pages instead of `layouts/alias.html`).
- `documentation/command-reference`: "One page per library" (rendered by
  the site build from the committed data instead of by Hugo).
- `documentation/api-reference`: "Built without warnings in the Pages
  workflow" (Doxygen into `site/public/api/` before the site build).

## Impact

- `site/` whole; `.github/workflows/pages.yml`; `Doxyfile`
  (`OUTPUT_DIRECTORY`); `.gitignore`; `cmake/site-fixoverview-test.cmake`,
  `cmake/api-doxygen-test.cmake` (path only), `site/tests/`;
  `tools/siteexamples/main.cpp` (directories scanned), its README and
  `tooltest.cmake`; `tests/site-examples/examples.txt`,
  `tests/site-examples/CMakeLists.txt` (the glob of the site files);
  `tests/README.md` ("Site examples"); for the keyword export a host tool
  in `tools/` (with `tools/lib/` and `tools/tests/`), its check in blitz2's
  build and `site/data/compiler/blitz2.json`; `site/README.md`,
  `docs/development.md`, `docs/structure.md`; `.claude/agents/site-writer.md`
  (only with the user's OK to the wording).
- New dependencies (npm, build time; MIT, Apache-2.0, ISC, BSD, MPL-2.0;
  sharp's libvips LGPL-3.0-or-later, build time only): Astro, Starlight,
  astro-mermaid, mermaid, yaml, mdast/unist utilities; Node.js at a
  pinned version (locally and in the tests the host's Node.js at that
  version or the pinned container image via podman; the container of the
  Pages job).
- Open changes that edit site pages: `ab3-mode-g5` (task 6.1,
  `ab3-compared/not-supported.md`), `console-break-option` (Extensions,
  `ab3-mode/command-line.md`, `rundebuglib.md`),
  `original-products-commands` (`lmullib.md`, `data/libintros`,
  `data/cmddesc`). No freeze (the user's answer of 2026-11-27, design.md
  "Decisions of the user", Q5): they edit Hugo pages until the cut-over.
- The user's answers of 2026-11-27 (design.md, "Decisions of the user"):
  the compiler keywords exported from `toklist.h` (Q1), a pinned Node.js
  container (Q3), no freeze (Q5). Still to be shown to the user: the
  wording for the site-writer agent and the memory note on Hugo (Q6).
