# Tasks

Until the cut-over (group 9) Hugo stays the published site and the
documented way to build it; the Hugo tests (`ctest -L site`) stay green
after every group. The documentation of the Starlight build is written
with the cut-over, when it becomes the state (group 9); each earlier
group documents its parts in the head comments of its files. The spike's
code (branch `worktree-agent-a60685d3b7056fa15`, d454d3d90,
`spike/site-generator/`) is the starting point; it is ported, not merged.
Work data (reports, counts, screenshots) goes to
`~/.local/share/bb2-c-work/site-starlight/`.

## 1. The Astro project next to Hugo

- [x] 1.1 `site/package.json` (private, `type: module`, exact versions of
  astro, @astrojs/starlight, @astrojs/markdown-remark, astro-mermaid,
  mermaid, yaml, mdast-util-from-markdown and the unist/mdast utilities
  the plugins need: the then-current releases of the spike's set),
  `site/package-lock.json`, `site/.npmrc` (`engine-strict=true`),
  `engines.node`; Node.js in the pinned
  `docker.io/library/node:<version>-bookworm-slim` image via podman
  (design, "Decisions of the user", Q3), `npm ci` and the build run in it. `site/.gitignore`: `node_modules/`, `dist/`, `.astro/`,
  `src/content/docs/docs/` (the converter's output until 9.1). Verify:
  `npm ci` in the container in a fresh clone leaves `git status` clean; `npm ls` shows only
  the pinned versions.
- [x] 1.2 Licences: list the licence of every package of the lock file
  (a one-off listing from the lock file) and which packages are shipped to
  the browser; every shipped one is GPL-3.0-compatible (`khroma`: check
  its repository). Verify: the list in the work directory; a package that
  is not compatible stops the task and goes to the coordinator (D14).
- [x] 1.3 `site/astro.config.mjs` (site `https://fubar-coder.github.io`,
  base `/bb2-c`, `trailingSlash: "always"`, `unified()` processor (D4),
  Starlight with title, GitHub link, `customCss` with
  `site/assets/css/*.css` in place; `publicDir: "static"` until 9.3,
  design D1), `site/src/content.config.mjs` with the
  docs collection, a placeholder home page; telemetry off. Verify: `npm run
  build` exits 0 and prints no warning (the empty i18n collection solved,
  D12); Hugo's build in its image is unchanged (`site_fixoverview` passes).
- [x] 1.4 The ctest `site_starlight` (label `site`): a CMake script that
  runs the Starlight build in the pinned Node.js image via podman, in a
  work directory of its own,
  and fails on a non-zero exit or any warning line; skipped (77) without
  podman or the image or when the dependencies cannot be installed.
  Verify: passes; a deliberately added warning (a test fixture) makes it
  fail; without podman or the image it is skipped.

## 2. The converter over all pages (gate)

- [x] 2.1 `site/scripts/convert-hugo.mjs` from the spike, for all pages:
  relrefs → site paths, `_index.md` → `index.md`, callout/details,
  Hextra shortcodes (cards, steps, filetree, hero, feature grid) → the
  forms of design D6 (`.mdx` only where a component is needed), the site
  shortcodes → leaf directives, front matter (`weight` → `sidebar.order`,
  `prev`/`next`, `aliases`, `anchorMoves`, `width`), code fences untouched;
  run by `npm run build` before Astro. Verify: the converted tree has the
  same number of ` ```blitzbasic ` blocks per page in the same order
  (`siteexamples check` on the converted tree with a manifest rewritten by
  the path rules of D15, in the work directory).
- [x] 2.2 Build all 193 converted pages (with the reference loader of 5.2
  stubbed if it is not done yet) and count every manual fix by class (MDX
  escapes, shortcode forms the converter misses, broken links, heading id
  differences, front matter, layout), each with the file and the kind of
  fix (converter rule, Hugo-neutral source edit, per-file patch: D2).
  Verify: the report `converter-run.md` in the work directory with the
  counts per class and the revised hours per remaining group.
- [x] 2.3 Heading ids: compare, per page, the ids of the Hugo build with
  those of the Starlight build (a one-off comparison, its output in the
  work directory); a difference gets an explicit `{#id}` in the Hugo page
  (Hugo-neutral) or a converter rule. Verify: the comparison shows no id
  of the Hugo build missing on its Starlight page, except the ids that the
  shortcodes of 5.1 generate (the 31 `#fixes-described-with-other-libraries`
  headings of the library fix pages; their comparison is part of 5.1's
  verification).
- [x] 2.4 Gate (design D3): the coordinator reads `converter-run.md`; if
  the counts or a class change the plan, the plan is revised with
  `openspec-update-change` before group 3; if the estimate exceeds 50 h,
  the coordinator asks the user before going on. Verify: the revised
  artifacts validate (`openspec validate site-starlight --strict`) or the
  report says that nothing changes.

## 3. Hextra to Starlight

- [x] 3.1 Asides and details: the four callout types and the details
  blocks render as Starlight asides and `<details>`. Verify: the pages
  with callouts (22) and details (5) in the built site show them; a
  screenshot of two of them, light and dark.
- [x] 3.2 Cards, steps, filetree on the 28 pages: `CardGrid`/`LinkCard`,
  `Steps`, `FileTree`; every card keeps its link target. Verify: the build
  passes; every card link resolves (link check of 4.1); screenshots of the
  chapter index pages.
- [x] 3.3 The home page: Starlight's splash template with the `hero`
  (headline, subtitle, button) and the feature cards. Verify:
  screenshots at 1400 and 420 px width.
- [x] 3.4 Sidebar: one group per chapter in the order of the chapters'
  weights, labelled with the chapter index titles; pages by
  `sidebar.order`; the reference groups collapsed; `prev`/`next` of the 25
  pages as today; the one `width: wide` page wide (schema field + CSS).
  Verify: the sidebar order of the built site equals Hugo's (a listing of
  both in the work directory); the pager of `expressions/` and
  `libraries/` names the neighbours of today.
- [x] 3.5 Mermaid on the 8 pages through astro-mermaid, loaded only on
  pages with a diagram. Verify: the diagrams render light and dark; a page
  without a diagram loads no mermaid chunk (the built HTML).

## 4. Links and the data layer

- [x] 4.1 The link check (design D7): every internal link of the pages and
  of the data, with its anchor, checked in the build; the base added by the
  site's remark plugin, and outside remark for the links that are no
  Markdown link nodes: the `href` of the `LinkCard`s, `prev`/`next` and
  the hero's actions (the pages keep site paths without the base). Verify: the build passes; broken-input tests
  `site_links_missing_page`, `site_links_missing_anchor`,
  `site_links_data` (a link in a `cmddesc` file) fail with the page and the
  link named (label `site`).
- [x] 4.2 `site/src/lib/data.mjs`: `site/data` read as today (no
  `BB2_SITE`), relrefs in `libintros`/`cmddesc` resolved until the
  cut-over. Verify: the 46 data links are site paths and pass the link
  check (39 now, 7 awaited from 5.2); that they appear in the output with
  the base is part of 5.2's verification, since only 5.2 renders the data.
- [x] 4.3 The leftover-syntax check (`{{<`, `{{%` outside code blocks in
  pages and data) fails the build, enabled by a setting that the cut-over
  (9.1) switches on for good. Verify: broken-input test
  `site_hugo_syntax` (a data file with a relref) fails naming the file.

## 5. The site's shortcodes and the command reference

- [x] 5.1 Remark plugin: `fixed-bugs-library` (head with the counts by
  level and the reference link; part `elsewhere`), `fixed-bugs-libraries`,
  `library-list`, the issue links of the library fix pages with the level
  badge and the command links (port of `render-link.html`), heading ids
  `{#id}`. Verify: the 120 library fix pages and the 4 + 3 index pages
  built; for windowslib, shapeslib, amigaos and one userlib the counts and
  links equal Hugo's (a comparison of the extracted lists in the work
  directory); the heading-id comparison of 2.3 over the library fix pages
  shows no id of the Hugo build missing (the 31
  `#fixes-described-with-other-libraries` headings).
- [x] 5.2 The command reference loader (port of `_content.gotmpl` and
  `cmdref/*`): one page per command library, AmigaOS library and userlib,
  with `libintros` and `cmddesc`, the per-command fix rows and "Fixed
  original bugs"; the AmigaOS call tables with break opportunities after
  each comma (port of `render-table.html`); command heading ids as Hugo's.
  Verify: the page count equals Hugo's (136 + 37); the heading ids of the
  reference pages equal Hugo's (the comparison of 2.3 over these pages);
  the widest AmigaOS table fits the content column (screenshot); the
  `AWAITED` entry for 5.2 is removed from `src/lib/links.mjs` and the link
  check passes without it; the 46 data links appear in the output with
  the base (4.2).

## 6. The overview of fixed bugs and its check

- [x] 6.1 `FixOverview.astro` (parts `levels`, `breaking`, `table`) with
  the markup of the Hugo shortcode; `assets/js/fix-overview.js` and
  `assets/css/fix-overview.css` unchanged. Verify: sorting, the filters,
  `?level=3,4` and `?topic=Graphics` work (browser check); with
  JavaScript off the rows are in the initial order and none hidden.
- [x] 6.2 Port of `fix-overview/check.html` into the build (design D9),
  same messages; `BB2_FIXOVERVIEW_DATA` selects a test data file; a
  failure removes `dist/`. `cmake/site-fixoverview-test.cmake` and
  `site/tests/CMakeLists.txt` run the Starlight build (Hugo's cases stay
  until 9.2 under other names or are switched here, whichever keeps both
  generators tested). Verify: `site_fixoverview` passes and the eight
  `site_fixoverview_*` cases fail with their messages; `dist/` is absent
  after a failed case.

## 7. Redirects and the old URLs

- [x] 7.1 Redirect pages from `aliases` in `astro:build:done` with the
  behaviour of `layouts/alias.html` (anchor kept, `anchorMoves`, meta
  refresh, `noindex`); an `anchorMoves` naming a missing page fails the
  build. Verify: `/bb2-c/docs/compared-with-original/fixed-bugs/compiler/#arrays-256`
  forwards to `language/#arrays-256` and the `anchorMoves` example of the
  spec to `editor` (browser check); broken-input test
  `site_alias_bad_move` fails naming the page.
- [x] 7.2 `site/tests/urls/hugo.txt`: every URL of the current Hugo build
  (pages and aliases), written once from the Hugo output; the build checks
  each against `dist/`. Verify: the build passes; broken-input test
  `site_urls_missing` (a list with one URL more) fails naming it.

## 8. Grammar, warnings, search, workflow

- [x] 8.1 The grammar (design D11) with the keywords exported from
  `toklist.h` (Q1): the host tool in `tools/` (logic in `tools/lib/`, a snitch test in
  `tools/tests/`), the committed `site/data/compiler/blitz2.json`, its
  check in blitz2's build step and a broken-input test, a `cmdref_update`-
  like target. Verify: a block with `If`, `NPrint`, a string, a comment
  and an AmigaOS call is highlighted (screenshot); the check fails with a
  changed token name in a test fixture; the export holds the `TK` names
  and none of the `TA` mnemonics; `ctest --preset host` and the tool test
  with `-DHOST_SANITIZE=ON` pass; no result changes (prev comparison). The
  14 `basic` fences (Blitz test-program fragments on 2 pages, outside the
  manifest, aliased to `plaintext` until then) get the grammar too or stay
  plain text: the decision is recorded in design.md.
- [ ] 8.2 No warnings in the full build (chunk size, D12); the
  modified-time meta tag only with `BB2_SITE_GITINFO=1` (D13); Pagefind
  excludes `public/api/`. Verify: `site_starlight` passes with the API
  reference in place; the meta tag is in a CI-style build and not in a
  plain one.
- [ ] 8.3 `.github/workflows/pages.yml` for Starlight (a new file content
  kept in the change until 9.3): Doxygen into `site/public/api` (the layout after 9.3), the build
  job's container the pinned Node.js image (Q3), `npm ci`, the build with `BB2_SITE_GITINFO=1`, upload of
  `site/dist`; the path filters as today. Verify: the same steps run
  locally in the pinned environment produce a `dist/` with `api/index.html`.

## 9. Visual review and cut-over

- [ ] 9.0 Visual review by the user: a local build of the whole site
  served next to the Hugo site (two ports), with a list of pages to look
  at (home, the chapter indexes, type-system, important-fixes with
  filters, two library fix pages, two reference pages, one AmigaOS page,
  a page with steps and filetree, dark mode, narrow width). Verify: the
  user's answer recorded by the coordinator; findings fixed before 9.1.
- [ ] 9.1 The converted tree committed: the final converter run over the
  pages as they are then (with the site edits of other changes merged
  by then: no freeze, Q5), the
  data files converted to site paths, the leftover-syntax check on, both
  ignore rules of the converted output removed (`src/content/docs/docs/`
  and `src/content/docs/index.mdx`). Verify: `site_starlight` and
  all `site_*` tests pass; `git grep -n '{{[<%]' site/src site/data` finds
  nothing outside code blocks.
- [ ] 9.2 Hugo removed: `hugo.yaml`, `go.mod`, `go.sum`, `layouts/`,
  `content/`, the converter, `.hugo_build.lock` from `.gitignore`; the
  Hugo variants of the site tests. `siteexamples` scans `src/content` and
  `data` (`tools/siteexamples/main.cpp`, its README and `tooltest.cmake`);
  `tests/site-examples/examples.txt` paths rewritten (names unchanged);
  the glob of `tests/site-examples/CMakeLists.txt`. Verify:
  `siteexamples check site tests/site-examples/examples.txt` passes with
  125 entries; `ctest --preset host`; the site example passes
  (`site_examples_run`, `site_examples_compile` and the rwe groups) with
  unchanged expected files; `-DHOST_SANITIZE=ON` for the tool test.
- [ ] 9.3 `pages.yml` switched (8.3); Astro's `publicDir` back to the
  default `public/` (`site/.gitignore`: Hugo's `public/`, `resources/`
  removed); `Doxyfile` `OUTPUT_DIRECTORY =
  site/public/api` and its comment; `.gitignore` `/site/public/api/`;
  `cmake/api-doxygen-test.cmake` if it names the path. Verify:
  `api_doxygen` and `api_doxygen_broken` pass; the workflow file is
  checked with the steps of 8.3.
- [ ] 9.4 Docs: `site/README.md` (preview, build, pins and their update,
  writing pages: site paths, asides, components, directives, raw HTML
  rule, the URL list, the checks and their tests), `docs/development.md`
  ("rendered by Hugo"), `docs/structure.md` (the `site/` line), the site
  page `project-structure/build-docs`, `tests/README.md` ("Site
  examples": paths), `tools/siteexamples/README.md`. The site-writer
  agent: the wording for "Hugo and Hextra" and "Checking" drafted in the
  work directory and shown to the user; changed only after the user's OK
  (Q6). Verify: every command in the docs run as written; `grep -rni
  'hugo\|hextra\|relref' site/README.md docs/ tests/README.md
  tools/siteexamples/README.md site/src` finds nothing that describes the
  current state.
- [ ] 9.5 Full acceptance with a clean rebuild (D-026: `all`, `bug-orig`,
  `originals`) and `ctest -L site`; no result changes (rule 3: the prev
  comparison passes without `-DPREV_INTENDED`). Verify: the logs'
  summaries in the work directory.

## Workflow follow-up

- The user pushes; then the coordinator checks the published site: the
  home page, an old URL with an anchor (`fixed-bugs/compiler/#arrays-256`),
  an `anchorMoves` case, the search, `/bb2-c/api/`, two reference pages.
- The site follow-ups that wait for the spike (`rounds/tree`, "2026-11-26
  Site generator spike starts; site follow-ups after it") come after the
  cut-over.
- Archive the change after the review.
