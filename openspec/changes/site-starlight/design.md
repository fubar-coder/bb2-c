# Design: the website on Astro Starlight

## Context

See proposal.md, "Why". Today:

- `site/` is a Hugo project (Hextra as a Hugo module, `go.mod`), built in
  the pinned image `docker.io/hugomods/hugo:go-git-0.165.0` with podman
  locally and as the container of the Pages job (pin raised by the user's
  answer "Bump the pin, use hugo.Data (Recommended)", `rounds/tree`,
  2026-11-26). It is not part of the CMake build; the ctests of the label
  `site` (`site/tests/CMakeLists.txt`) run Hugo in podman and are skipped
  without it.
- Site-specific Hugo code: 4 shortcodes, 17 partials (`cmdref/`,
  `fixed-bugs/`, `fix-overview/check.html`), 3 render hooks (blitzbasic
  code blocks as plain text, issue links with badges on the library pages,
  the AmigaOS call tables), the content adapter
  `content/docs/libraries/_content.gotmpl`, `layouts/alias.html`.
- Content: 193 pages, 649 relrefs in 144 files, 46 relrefs in 39 data
  files; Hextra shortcodes on 28 pages (cards, steps, filetree, hero,
  feature grid) plus 22 callouts and 5 details; 227 uses of
  `fixed-bugs-library`; 125 pages with `aliases`, 3 with `anchorMoves`;
  25 with `prev`/`next`; 1 with `width: wide`; 8 with Mermaid.
- `tools/siteexamples` scans `site/{content,data,layouts,assets}`; the
  manifest names 124 blocks by path and number.
- The spike (branch `worktree-agent-a60685d3b7056fa15`, d454d3d90,
  `spike/site-generator/`) has working prototypes of the data layer
  (`common/data.mjs`, `cmdref.mjs`), the converter (`convert-hugo.mjs`),
  the grammar (`grammar.mjs`), the remark plugin, the content loader and
  the overview component, on Starlight 0.43.2 / Astro 7.4.13 with
  `unified()` as the Markdown processor.
- Starlight facts read in its 0.43.2 sources (`dist/loaders.js`,
  `dist/utils/collection.js`): the docs collection is `<srcDir>/content/docs`,
  and its loader skips files whose name begins with `_` (Hugo's
  `_index.md`).

## Goals / Non-Goals

**Goals:** the site reads and looks as today or better, at the same URLs
and anchors; the same data files drive it; every check of today is kept
with its broken-input test; Hugo is gone at the end.

**Non-Goals:** see proposal.md ("Not in scope"). No change of a page's
text other than what the converter or a manual fix needs.

## Decisions

Each choice below is the planning agent's (D-011 (b)), rated with the
catalog; the coordinator records them in the round's
`coordinator-decisions.md`.

### D1. Layout: the Starlight default, `site/` as the Astro root

`site/package.json`, `site/astro.config.mjs`, `site/src/` (components,
plugins, styles, the content loader), the pages in
`site/src/content/docs/` with the home page as `index.mdx` and the docs
below `docs/` (so `src/content/docs/docs/compared-with-original/...`
serves `/bb2-c/docs/compared-with-original/...`), `site/data/` unchanged,
`site/public/` for static files (the API reference), output `site/dist/`.
Hugo's `_index.md` becomes `index.md` (the loader skips `_` files).
Until the cut-over Hugo writes its output into `site/public/` (ignored),
which Astro would copy as static files: Astro's `publicDir` is `static/`
(Hugo's static directory, which holds the API reference today) until the
cut-over and `public/` from then on (task 9.3).
Alternative: `srcDir: "."` with a `generateId` that prefixes `docs/`, so
that `site/content/docs/` keeps its paths; rejected: it depends on two
non-default settings Starlight's sidebar and git-date code assume
otherwise (its own comment in `collection.js`), and the paths change
anyway for `_index.md` and the `.mdx` pages.

Point: the default layout turns out wrong (the user wants the old paths,
or a later Starlight supports a content folder) — L2 Possible / I2 Medium /
E3 Proven (Starlight 0.43.2 sources) → Plan

### D2. Hugo pages stay the source until the cut-over

Until the cut-over, the converter runs before each Astro build and writes
the pages into `site/src/content/docs/` (ignored by git, as in the
spike); Hugo and the Pages workflow stay as they are. Manual fixes are
made (a) as converter rules, (b) as edits of the Hugo page that render
the same in Hugo (an explicit `{#id}`, an escaped brace), or (c) as a
per-file patch the converter applies. At the cut-over the converter runs
once more over the pages as they are then, its output is committed, and
the converter, Hugo's files and the ignore rule go in the same commit.
So other changes can edit Hugo pages until the cut-over, and the cut-over
commit is reproducible from the converter. Alternative: convert and
commit early, keep both trees; rejected: two sources for every page, and
`siteexamples` would count every block twice.

Point: a manual fix that the converter cannot express — L2 Possible /
I1 Low / E2 Reasoned → Monitor (task 2.2 counts them)

### D3. One change with a gate after the converter run (Q4)

One change, in task groups that each end with a buildable state; group 2
(the converter run over all 193 pages and the data files) ends with a
report and a gate: the plan is revised with `openspec-update-change`
before group 3 if the count of manual fixes or a class of problems
changes it, and the coordinator takes it to the user if the estimate
grows beyond 40 h by more than a quarter (the user decided on the spike's
30-40 h). Alternative: a series (scaffold + data layer; content; checks
and redirects; cut-over). Rejected: a series sets an order between
changes (I4) for work that is ordered anyway, and the cut-over needs all
parts at once.

Point: the change gets too large for one review, so that it must be
split later — L2 Possible / I2 Medium / E2 Reasoned → Plan

### D4. The Markdown processor: `unified()` (Q2)

`markdown.processor: unified({ remarkPlugins, rehypePlugins })` as in the
spike, not Astro 7's default engine (Sätteri), whose plugin API is its
own and which astro-mermaid and the remark ecosystem (directives, mdast
utilities) do not use. The site's plugins are kept small and in
`site/src/plugins/` so that a later move touches one directory.
Alternative: Sätteri now; rejected: no mermaid integration and no
directive support known for it, more porting now for no reader-visible
gain.

Point: a later Astro major removes `unified()` and forces a port of the
site's plugins — L2 Possible / I2 Medium / E2 Reasoned (works on 7.4.13,
E3 by the spike; the future is not checked) → Plan

### D5. Pinning

Exact versions in `package.json` (no `^`/`~`), `package-lock.json`
committed, `npm ci` only, `.npmrc` with `engine-strict=true` and the
Node.js version in `engines`; Astro's telemetry off
(`ASTRO_TELEMETRY_DISABLED=1` in every documented call and in the
workflow). An update is its own commit, as for Hextra today. The versions
are the then-current releases of the spike's set, taken in task 1.1.
Node.js runs in the pinned container image of Q3 ("Decisions of the
user"); its tag is the pin of the Node.js version.

Point: a pinned version has a known security fix pending — L2 Possible /
I1 Low / E2 Reasoned → Monitor (the build runs on trusted content only)

### D6. Content forms

- Pages stay `.md` where they can; `.mdx` only for the pages that need a
  Starlight component (cards → `CardGrid`/`LinkCard`, steps → `Steps`,
  filetree → `FileTree`; the home page with the splash template and its
  `hero`; `important-fixes` with `FixOverview`): about 30 files.
- callout → asides (`:::note`, `:::caution`, `:::danger`, `:::tip`, the
  mapping of the spike); details → `<details><summary>`, styled by
  Starlight.
- The site's shortcodes → remark leaf directives handled by the site's
  plugin (`::fixed-bugs-library{part="elsewhere"}`,
  `::fixed-bugs-libraries{group="commands"}`, `::library-list{...}`),
  so that the 120 library pages stay `.md`.
- Explicit heading ids `{#id}` stay (the spike's plugin); the ids of all
  pages are compared with the Hugo build (task 2.3) and a difference is
  fixed by an explicit id.
- `weight` → `sidebar.order`; `prev`/`next` → Starlight's `prev`/`next`
  with site paths; `aliases`, `anchorMoves` stay in the front matter
  (the schema is extended); `width: wide` (one page) → a front-matter
  field of the extended schema and a CSS rule.
- Sidebar: one group per chapter, labelled with the chapter's index
  title, autogenerated below; the command reference groups collapsed.

Point: the MDX pages need escapes (`{`, `<`) that change their text —
L3 Likely / I1 Low / E3 Proven (spike: "MDX escaping on ~30 pages") →
Monitor (task 2.2 counts them, the visual review checks them)

### D7. Links

Links in pages and data are site paths without the base (`/docs/...`);
the site's remark plugin adds the base. The build checks every internal
link and its anchor (a remark/rehype step over the collected heading ids,
or the `starlight-links-validator` plugin if it accepts the based links;
task 4.1 picks one and names it), with a broken-input test. A relref
that the converter leaves (or that a writer writes after the cut-over)
fails the build (requirement "No syntax of the former generator").

Point: the chosen validator misses a class of links (relative links,
links in data files) — L2 Possible / I1 Low / E2 Reasoned → Monitor (the
broken-input tests cover a page link, a data link and an anchor)

### D8. Data layer

`site/src/lib/data.mjs` (from the spike's `common/data.mjs`, without the
`BB2_SITE` work-around: the path comes from the Astro root) reads
`site/data` as today: `libraries/*.json`, `libintros`, `cmddesc`,
`fixoverview.yaml`. Until the cut-over it resolves relrefs in the data
(the spike's `resolveRelrefs`); at the cut-over the data files are
converted to site paths in the same commit and the resolver goes.
The command reference is a content loader (the spike's), with entries
that carry a `filePath` under the docs collection so that the sidebar
places them; the AmigaOS pages' call tables get break opportunities after
each comma (the port of `render-table.html`) and the heading ids of the
commands are Hugo's (spec `documentation/command-reference`).

### D9. Checks in the build step of the site

The overview check (port of `check.html`), the URL list check, the
`anchorMoves` check and the leftover-syntax check run in the site build
(an Astro integration; the checks on page ids in `astro:build:done` over
the output HTML, which is what a reader gets). A failing check throws,
so `astro build` exits non-zero, and removes `dist/` first (CLAUDE.md:
a failed check means the result is not built). The broken-input tests
use an environment variable that names a test data file
(`BB2_FIXOVERVIEW_DATA=tests/fixoverview/data/CASE.yaml`, the port of
the Hugo parameter `fixOverviewData`); test names and messages stay.

### D10. Redirect pages and the old URL list

The redirect pages are written in `astro:build:done` from the `aliases`
of the collection, with the HTML of `layouts/alias.html` (JavaScript keeps
the anchor, `anchorMoves` sends a moved entry on, `meta refresh` and a
link without JavaScript, `noindex`). Before the cut-over, the list of every
URL of the Hugo build (pages and aliases, about 520) is written from
the Hugo output once and committed as `site/tests/urls/hugo.txt`; the
build checks that each is in `dist/`. Alternative: Astro's `redirects`
config; rejected: it keeps no anchor and has no `anchorMoves`.

Point: the URL list freezes a page that is later removed on purpose —
L2 Possible / I1 Low / E2 Reasoned → Monitor (the removal deletes its
line in the same commit; site/README.md says so)

### D11. Grammar

`site/src/lib/grammar.mjs` from the spike: commands, objects and AmigaOS
calls from `site/data/libraries`; the compiler keywords from
`site/data/compiler/blitz2.json`, the export of Q1 ("Decisions of the
user"). Registered as `expressiveCode.shiki.langs`, aliases `bb2`,
`blitz2`. The blitzbasic render hook of Hugo (plain text) goes.

### D12. Build warnings

The empty i18n collection: declare no `i18n` collection or give it the
loader with an empty glob that Starlight accepts without the warning
(task 1.3 finds which one is warning-free); the chunk-size warning:
mermaid is loaded only on pages with a diagram (astro-mermaid), and the
`build.chunkSizeWarningLimit` of Vite is raised only for the mermaid
chunk with a comment naming its size. Any other warning is fixed at its
cause.

Point: raising the chunk limit hides a later real size regression —
L2 Possible / I1 Low / E2 Reasoned → Monitor (the limit is set just
above the mermaid chunk's size)

### D13. Last-modified dates and search

Today the git dates are meta tags of the CI build only, not shown.
Starlight's `lastUpdated` shows a visible date; it stays off, and the
meta tag is written by a `Head` override from git in the CI build only
(`BB2_SITE_GITINFO=1`), as `--enableGitInfo` today. Search: Starlight's
Pagefind replaces Hextra's FlexSearch; `public/api/` is excluded from the
index.

Point: the user wants the date shown — L1 Unlikely / I1 Low / E2
Reasoned → Monitor

### D14. Licences

Before the first commit of the lock file the licences of every package
in it are listed (task 1.2); everything that is shipped to the browser
(Starlight's scripts, Pagefind's UI, mermaid and its dependencies, among
them `khroma`, which has no licence field in the lock file) must be
GPL-3.0-compatible. Build-time tools need only be usable.

Point: a shipped package has a GPL-incompatible licence — L1 Unlikely /
I5 Critical / E2 Reasoned (the spike's lock: MIT, ISC, BSD, Apache-2.0,
MPL-2.0, LGPL-3.0 for sharp's libvips at build time, one without a field)
→ Decide if found (task 1.2 stops and reports)

### D15. Site examples

`siteexamples` scans `site/src/content` and `site/data` (and no longer
`layouts`, `assets`; never `node_modules`, `dist`, `.astro`). At the
cut-over the manifest's paths are rewritten (`content/docs/X.md` →
`src/content/docs/docs/X.md`, `_index.md` → `index.md`, `.mdx` where
the page became one); names stay, so the expected outputs stay. The
converter must keep every ` ```blitzbasic ` block and its order (task 2.1
checks: `siteexamples check` on the converted tree with the rewritten
manifest).

## Risks / Trade-offs

- [Astro majors are frequent; Sätteri moves] → exact pins, `unified()`
  (D4), the plugins in one directory.
- [Heading ids differ between Hugo's and Starlight's sluggers] → the id
  comparison of task 2.3; explicit ids where they differ; the overview
  check proves every row anchor.
- [MDX escapes on ~30 pages] → `.md` wherever possible (D6); the visual
  review.
- [Generated reference entries need a made-up `filePath`] → kept from
  the spike, with a comment; checked by the sidebar of the built site.
- [Two generators in `site/` until the cut-over] → Hugo ignores `src/`,
  `node_modules/`, `dist/`; Astro ignores `content/`, `layouts/`; the
  Hugo tests stay green until the cut-over (task 3.x verifies both).

## Migration Plan

1. Groups 1-2: the Astro project next to Hugo, the converter over all
   pages, the report and the gate (D3).
2. Groups 3-7: components, data, checks, redirects, grammar, warnings;
   Hugo stays published.
3. Group 8: the visual review by the user on a local build of the whole
   site next to the Hugo site.
4. Group 9, the cut-over, in one commit (or one short series pushed
   together): final converter run, the converted pages and data
   committed, Hugo files, converter and Hugo tests removed, the manifest
   rewritten, `pages.yml` switched, the docs updated; then the full
   acceptance (D-026) and the user's push. Rollback: revert the cut-over
   commit(s); the Hugo site is complete in the parent commit.
5. After the push: the published site checked (old URLs with anchors,
   search, API reference).

## Decisions of the user

The user's answers of 2026-11-27 (`rounds/tree/decisions.md`,
"site-starlight: keywords, Node, order, commit"); D3 and D4 stand.

- **Q1 The compiler keywords for the grammar:** "Export from toklist.h
  (Recommended)". A host tool in `tools/` (logic in `tools/lib/`, its test
  in `tools/tests/`) writes the compiler's tokens from
  `programs/blitz2/src/toklist.h` (its `TK` entries are the BASIC
  keywords, its `TA` entries the assembler mnemonics, so no hand
  classification) to a committed `site/data/compiler/blitz2.json`,
  checked in blitz2's build step like the command reference JSON, with a
  broken-input test and an update target like `cmdref_update`.
- **Q3 Where Node.js runs:** "Pinned container (Recommended)". A pinned
  `docker.io/library/node:<version>-bookworm-slim` image (glibc, so the
  native packages are those of a glibc host) via podman locally (preview
  and build) and in the ctests, and as the container of the Pages build
  job. The tests are skipped (77) without podman or the image.
- **Q5 The order with the open changes that edit site pages**
  (`ab3-mode-g5` 6.1, `console-break-option`,
  `original-products-commands`): "No freeze (Recommended)". They edit
  Hugo pages until the cut-over and the final converter run (9.1) takes
  their state; a site task of theirs still open at the cut-over is done
  in the new form, its tasks.md adjusted by the coordinator.

## Open questions for the user

- **Q6 Wording outside the repository's docs.** The site-writer agent
  (`.claude/agents/site-writer.md`, section "Hugo and Hextra" and the
  build command of "Checking") and the memory note "site-readability"
  ("hugo runs via podman") need new wording; CLAUDE.md names no Hugo and
  stays. Task 9.4 drafts the agent's wording and shows it; it is changed
  only with the user's OK (rule 1). The memory note is the coordinator's,
  with the user's OK.

## Findings

### Group 1 (2026-11-27)

- **Versions (1.1).** The then-current releases of the spike's set:
  astro 7.4.13, @astrojs/starlight 0.43.2, @astrojs/markdown-remark
  7.3.10, astro-mermaid 2.2.0, mdast-util-from-markdown 2.1.0, yaml 2.9.1
  (all as in the spike) and mermaid 12.3.0 (the spike had 11.17.2;
  astro-mermaid 2.2.0 accepts `^12`). The spike's plugins need no other
  unist/mdast utility (the remark plugin walks the tree itself; the
  directives come with Starlight's remark-directive). Node.js 26.13.1, the
  Active LTS ("Lithium"), in `docker.io/library/node:26.13.1-bookworm-slim`
  (npm 11.20.0); astro 7.4.13 needs Node.js >= 22.23.4. `engines.node` is
  the image's version exactly.
- **Licences (1.2).** The listing is
  `~/.local/share/bb2-c-work/site-starlight/licences.md`. Shipped to the
  browser (the closure of mermaid and the client code of Starlight,
  Pagefind, Expressive Code, astro and astro-mermaid): MIT, ISC,
  BSD-3-Clause, Apache-2.0, Unlicense, dompurify MPL-2.0 OR Apache-2.0,
  elkjs EPL-2.0 OR GPL-3.0-or-later; `khroma` 2.1.0 has no licence field,
  its repository and its package carry the MIT licence. All
  GPL-3.0-compatible. Build time only: MPL-2.0 (lightningcss),
  LGPL-3.0-or-later (sharp's libvips), BlueOak-1.0.0, CC0-1.0, 0BSD.
- **The i18n warning (1.3, D12).** Neither option of D12 is free of the
  warning: Starlight reads the i18n collection on every page, and Astro
  7.4.13 warns for a collection that is not in its data store, which an
  undeclared collection and a glob without a match alike are not (the
  store holds only collections with an entry; Starlight's own
  `console.warn` muting does not reach Astro's logger). The collection is
  declared with Starlight's `i18nLoader()` and holds
  `src/content/i18n/en.json` with no override (`{}`).
- **The 404 page (1.3).** Without a docs entry `404` Astro warns "Entry
  docs → 404 was not found" (Starlight's 404 route looks it up); a plain
  `src/content/docs/404.md` is also rendered by Starlight's `[...slug]`
  route, for which Astro warns of a route conflict. `404.md` has
  `draft: true`: the production build leaves drafts out of the page routes,
  and the 404 route still takes the entry.
- **Telemetry.** `ASTRO_TELEMETRY_DISABLED=1` is set in the package
  scripts (`build`, `preview`), so every call through npm has it.

### Group 2 (2026-11-27)

- **The converter run (2.1, 2.2).** The report is
  `~/.local/share/bb2-c-work/site-starlight/converter-run.md`. All 193
  pages convert and build without a warning (165 `.md`, 28 `.mdx`); every
  fixed code block (527) is unchanged and in its place; no Hugo source
  edit and no per-file patch was needed, every fix is a converter rule or
  configuration. MDX escapes: none (the risk of D6 did not occur).
- **Steps.** Hextra takes the headings of the first level in a steps
  block as the steps (`####` on `self-judging-tests`); filetree and cards
  are converted before steps, which indents them into the list items.
- **The home page** is converted too (`src/content/docs/index.mdx`, the
  splash template with the hero in the front matter); the placeholder of
  1.3 is gone and the file is ignored like the converted tree (9.1 removes
  both rules).
- **Languages.** Expressive Code warns for a fence language it does not
  know: `blitzbasic` (until the grammar, 8.1) and `basic` (14 fragments of
  test programs on two test pages, outside the site-examples manifest) are
  aliased to `plaintext`; 8.1 decides on `basic`.
- **The base outside remark.** The `href` of the 80 `LinkCard`s (MDX JSX
  attributes), `prev`/`next` (front matter, used by Starlight as written)
  and the hero's action are no Markdown links: the base step of 4.1 must
  reach them, or they keep a site path without the base.
- **Heading ids (2.3).** Hugo's and Starlight's ids agree on every
  heading the pages write (no explicit `{#id}` needed); the only missing
  ids are the 31 `fixes-described-with-other-libraries` that the
  `elsewhere` part of `fixed-bugs-library` writes from the data (5.1).
  Unhandled directives are shown as text until 5.1 and 6.1 (no warning).
- **FileTree** opens every folder that has children: Hugo's closed
  `Sourcecodes` folder (`ab3-mode/setup`) shows open.
- The manifest has 125 entries (9.2 names 124).
