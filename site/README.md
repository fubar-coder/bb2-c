# bb2-c website

Source of the project website, <https://fubar-coder.github.io/bb2-c/>. It is
an [Astro](https://astro.build/) site with the
[Starlight](https://starlight.astro.build/) documentation theme. It is not
part of the CMake build; GitHub Actions builds and publishes it
(`.github/workflows/pages.yml`) on every push to `main` that touches `site/`.

The steps to build the site, the API reference and the command reference,
and how the Pages workflow does it, are also on the site itself:
<https://fubar-coder.github.io/bb2-c/docs/project-structure/build-docs/>.

The site is written for readers of the project, not for browsing the
repository. `docs/` stays the internal developer documentation and is the
source of facts, but is not included here; the pages are new texts.

## Layout

* `src/content/docs/`: the pages. `index.mdx` is the home page (Starlight's
  splash template, its hero in the front matter); `docs/` holds the
  chapters, so `src/content/docs/docs/libraries/index.mdx` is served as
  `/bb2-c/docs/libraries/`. `404.md` is the page for a missing URL.
* `data/`: the command tables of the libraries, their hand-written
  introductions and descriptions, the overview of fixed bugs and the
  compiler's keywords for the highlighting (below).
* `src/`: the site's code: `content.config.mjs` (the collections and the
  front matter), `components/`, `plugins/` (the remark plugins),
  `lib/` (data, command reference, sidebar, checks, redirects, grammar),
  `integrations/site-checks.mjs` (the checks of the build), `styles/`.
* `assets/`: the script and the styles of the overview of fixed bugs and
  of the level badges.
* `tests/`: the inputs of the site's tests (below).
* `public/`: static files served as they are, below the base: the site's
  icon `favicon.svg`.
* Written by the build and not committed: `node_modules/`, `.astro/`,
  `dist/` (the built site) and `public/api/` (the API reference).

## Node.js and the pinned versions

The site is built with Node.js 26.13.1, the exact version of
`engines.node` in `package.json`; `.npmrc` has `engine-strict=true`, so
`npm` refuses another version. Every dependency has an exact version in
`package.json`, and `package-lock.json` is committed; install only with
`npm ci`, which installs the versions of the lock file and does not
change it. The package scripts turn Astro's telemetry off
(`ASTRO_TELEMETRY_DISABLED=1`).

There are two ways to run it, with the same result:

* **On the host**, when `node --version` prints `v26.13.1`: run `npm` in
  `site/`.
* **In the pinned container image**
  `docker.io/library/node:26.13.1-bookworm-slim` (its tag is the version
  of `engines.node`) via podman, with `site/` mounted. The Pages workflow
  builds in the same image. Add `:Z` to the volume on SELinux hosts.

Either way installs the dependencies into `site/node_modules/`; after
switching from one way to the other, install again with `npm ci`.

### On the host

In `site/`:

```
npm ci            # the dependencies of the lock file
npm run dev       # dev server: http://localhost:4321/bb2-c/
npm run build     # the site, into dist/
npm run preview   # serves dist/: http://localhost:4321/bb2-c/
```

### In the container

From the repository's root:

```
podman run --rm -v $PWD/site:/site -w /site \
    docker.io/library/node:26.13.1-bookworm-slim npm ci
podman run --rm -it -p 4321:4321 -v $PWD/site:/site -w /site \
    docker.io/library/node:26.13.1-bookworm-slim npm run dev
podman run --rm -v $PWD/site:/site -w /site \
    docker.io/library/node:26.13.1-bookworm-slim npm run build
podman run --rm -it -p 4321:4321 -v $PWD/site:/site -w /site \
    docker.io/library/node:26.13.1-bookworm-slim npm run preview
```

The servers of the container answer on IPv4 only: open
`http://127.0.0.1:4321/bb2-c/` (a `localhost` that resolves to `::1`
gets no answer).

The dev server (`astro dev`) renders each page when it is requested and
shows a changed page of `src/content` at the next request (the browser
reloads it); its port is 4321 on all interfaces. Stop it with Ctrl-C. The checks run only in a full build (below), so a
change is done when `npm run build` passes. The preview serves the last
build as it is.

When Astro finds that it is run by an AI agent, `npm run dev` and
`npm run preview` start their server in the background and return; then
`npx astro dev stop` (or `npx astro preview stop`) in `site/` stops it.

The build must finish without errors or warnings. The "last modified"
dates of the pages (their meta tags `article:modified_time`, not shown on
the page) come from git and are written only by the CI build
(`BB2_SITE_GITINFO=1`, `.github/workflows/pages.yml`); a local build and
the tests need no git.

### The tests: `SITE_NODE`

The ctests of the label `site` (`ctest --test-dir build/amiga -L site`;
they are part of `ctest --preset host`) build a copy of `site/` with
`npm ci` and `npm run build` (`cmake/site-starlight-test.cmake`). The
CMake cache variable `SITE_NODE` chooses where Node.js runs:

* `auto` (the default): the host's Node.js when its version is the
  pinned one, else the pinned image via podman; the tests are skipped
  when neither is available.
* `host` or `container`: only that way; the tests fail, naming what is
  missing, when it is not available.

```
cmake --preset amiga -DSITE_NODE=container
```

### Updating the pins

An update is a commit of its own; no build updates a dependency.

* A dependency: in `site/`, with Node.js at the pinned version,
  `npm install --save-exact NAME@VERSION` (it writes `package.json` and
  `package-lock.json`). List the licences of the new packages in the lock
  file: what is shipped to the browser must be GPL-3.0-compatible.
* Node.js: the new version in `engines.node` (`package.json`), the image
  tag in `cmake/site-starlight-test.cmake` and in
  `.github/workflows/pages.yml`, and the version in this README, in the
  head comment of `astro.config.mjs` and on the site's page
  `project-structure/build-docs`; then `npm ci` with the new version.

Then `ctest --test-dir build/amiga -L site` must pass, and the built site
is looked at.

## The command reference

The pages under `/docs/libraries/commands/`, `amigaos/` and `userlibs/`
are generated by the build: the loader of `src/content.config.mjs`
(`src/lib/cmdref.mjs`) makes one page per library from the command tables
in `data/libraries/GROUP/NAME.json`. Only the sections' index pages
(`src/content/docs/docs/libraries/*/index.md(x)`) are written by hand;
the directive `::library-list{what="commands"}` lists the libraries of a
section there.

The JSON files are generated from the built library files by the host
tool `blitzlib cmdref` (`tools/blitzlib/README.md`) and committed. The
CMake build checks in the build step of every library that the committed file equals the generated one,
so a change of a library's declarations fails the build until the files
are updated: build the target `cmdref_update`, which copies the generated
files here, and commit them with the change. Do not edit them by hand.

A hand-written description and introduction of a library go into
`data/libintros/GROUP/NAME.yaml` (GROUP `acidlibs`, `deflibs` or
`userlibs`; for an AmigaOS library NAME is the deflibs library that opens
it, as `doslib`):

```
description: One sentence, for the library lists and search.
intro: |
  Markdown, shown above the commands.
```

Without the file, the page has a generated description and no
introduction.

A hand-written description of single commands goes into
`data/cmddesc/GROUP/NAME.yaml` (GROUP `acidlibs` or `userlibs`, NAME the
library), keyed by the command's name as the library declares it
(quote names with `$`):

```
"Left$":
  text: The first `Length` characters of `String$`.
  notes: |
    Markdown, shown after the command's forms (optional).
```

`text` (one or two sentences) is shown under the command's heading,
`notes` after its forms. A command without an entry shows only what the
library declares. Every statement must be checked against the library's
source or a test; a kept bug of the original names its issue as a link
(`([#285](https://github.com/fubar-coder/bb2-c/issues/285))`).
`src/lib/cmdref.mjs` merges both files into the pages. Links in both files
are site paths, as on the pages (below).

In both files, a plain (unquoted) value must not contain `": "`: YAML
reads it as a new key and the build fails ("mapping value is not
allowed"). Quote such a value, or write it as a block scalar (`|` or
`>`), as `intro` and `notes` above.

The compiler's keywords for the highlighting of Blitz Basic code are in
`data/compiler/blitz2.json`, written from blitz2's token list by the host
tool `toklist` and checked in blitz2's build step; after a change of the
token list, build the target `blitz2_keywords_update` and commit the file.

## The chapter "Differences from the original"

The chapter (`src/content/docs/docs/compared-with-original/`) has these
pages, in this order: "Important Fixes & Breaking Changes"
(`important-fixes.mdx`, the overview table below), the pages of fixed
bugs "Language" (`language.md`: how the compiler reads a source and what
it accepts), "Expressions" (`expressions/index.md`: the value of an
expression, with the sub-page "Expression type system",
`expressions/type-system.md`), "Compiled programs"
(`compiled-programs.md`: the program the compiler generates), "Running
blitz2" (`running-blitz2.md`: blitz2 as a program), "Editor"
(`editor/index.md`) and "Libraries" (`libraries/`, with its groups
`commands/`, `userlibs/` and `amigaos.md`), then "Extensions", "Bugs not
fixed yet" and "Comparing with the original yourself". A fix of the
compiler goes to Language when it is about what a source may contain, to
Expressions when it is about the value of an expression, and to Compiled
programs when it is about the generated program. A fixed bug is
described once, on its page of fixed bugs; a fix that makes the port
refuse a source or stop with a new runtime error (levels 3 and 4) is
described there with its example and what to change in a source.

Starlight's pager ("Previous", "Next" under a page) follows the sidebar.
Where the reading order differs (the pages next to a section, such as
`expressions/` and `libraries/`), a page names its neighbours in `prev`
and `next` in its front matter (`{link, label}`, the link a site path).

On the pages of fixed bugs of the compiler, the fixes that an ordinary
program can meet are sections of their own with a program. The others
are under "Other fixes", in groups (`###`), each with a table
`What was wrong | Now | Issue` (one short sentence per cell, the issue
as a link `[#N](URL)`). A fix of level 3 or 4, or one that needs more
room, follows the table of its group as an entry with its own heading
(`####` under the group's `###`): the program with comments for the
original's and the port's results, the reason, and **What to change**.
A form that only the port accepts, as a side of a fix, is on
"Extensions", under "Forms the original refused".

A page keeps the URLs that readers may have bookmarked as `aliases` in
its front matter (the pages of fixed bugs: below `fixed-bugs/`; the
pages below `new-errors/`, whose entries are on the pages of fixed
bugs and on Extensions). The build writes a page at each such URL
(`src/lib/redirects.mjs`); it forwards to the page and keeps the anchor
of the link. An entry that went to another page than the alias names is
listed in the front matter `anchorMoves` of the alias's page, by its
anchor (`frac-string: libraries/commands/vallib`, a page below
`/docs/compared-with-original/`), and a link to that anchor goes there;
an `anchorMoves` naming a page that is not built fails the build. A page
that moves gets an alias for its old URL, and its headings keep their
ids. The old page "Compiling" (`compiler/`) is an alias of Language,
whose `anchorMoves` send its entries on Expressions, Compiled programs
and Running blitz2 there.

## The overview of fixed bugs

The page "Important Fixes & Breaking Changes"
(`src/content/docs/docs/compared-with-original/important-fixes.mdx`)
shows one table row per fixed original bug. The rows are in
`data/fixoverview.yaml`; the component `FixOverview`
(`src/components/FixOverview.astro`, `src/lib/fix-overview.mjs`) renders
them (`<FixOverview part="levels" />`, `part="breaking"`,
`part="table"`), and `assets/js/fix-overview.js` and
`assets/css/fix-overview.css` add sorting and filters. The file has four
parts:

* `levels`: the five severity levels, each with `level` (1-5), `name` and
  `text`, shown on the page and in the filter.
* `topics`: the list of topics a row may have.
* `notRows`: issues linked on a page of fixed bugs that are not a row of their
  own (a cross-reference to a kept bug, for example), each with `issue` and
  `reason`.
* `rows`: one entry per fixed issue:

  ```
  - issue: 421
    severity: 1          # 1-5, the levels at the top of this file
    topic: "Graphics"    # one of topics
    library: "polylib"   # the library of the command reference (several
                         # separated by ", "), or "Compiler" or "TED"
    command: "RenderToBitmap, RenderPoly"   # may be ""
    text: "One sentence for the reader: what the port does, and what the original did."
    short: "Draws on the first bitmap, with the second as the pattern."
    page: libraries/commands/polylib   # the page of fixed bugs: language,
                         # expressions, compiled-programs,
                         # running-blitz2, editor, libraries (the index),
                         # libraries/commands/NAME, libraries/userlibs/NAME
                         # or libraries/amigaos
    anchor: rendertobitmap-dither   # a heading id on that page
    highlight: true      # optional: the row is in the warning box
  ```

  The level answers one question: can a program that ran correctly with
  the original behave differently with the port, and how hard is that to
  notice beforehand? Level 1 is a different result that matters (computed
  values, file contents, the way through the program, what it draws or
  prints, or a stop); level 2 a difference a user may notice while the
  program still does its job (sound, timing, display details, the order
  or form of output). A new runtime error (3) ranks above a new compile
  error (4) because it is harder to foresee. A level-3 sentence names the
  runtime error in at most five words. Quote every text value.

  `short` is the row's sentence in the table: one short sentence (about
  40 to 80 characters) with the key fact of `text`, what the port does.
  The table shows the row's commands right above it, so a sentence about
  the one command of its row does not begin by naming it. `text` stays
  the full sentence: the warning box, the command reference and the
  lists "Fixes described with other libraries" show it.

  The table's Component column is made from `library`: "Compiler" is
  shown as blitz2 (also for the rows of the page running-blitz2), "TED"
  as ted, a deflibs library of the page `libraries/amigaos` as the AmigaOS
  library it opens (`execlib` as `exec.library`), any other name as it
  is. The URL parameters `?level=3,4` and `?topic=Name` set the filters
  when the page opens (`assets/js/fix-overview.js`).

  The page's warning box shows only the rows with `highlight: true` (a
  boolean; any level), the few fixes an ordinary program meets most
  easily, and points to the table for the count of all rows of levels 1
  and 2. Keep it to about three to six rows.

Every fix that gets an entry on a page of fixed bugs also gets its row here.
The build checks the file (`checkFixOverview` in `src/lib/fix-overview.mjs`)
and fails when the levels are not the five levels 1 to 5, when a row lacks
a field, has an unknown level or topic, names an
anchor that is no heading of its page, or appears twice, and when an issue
linked on a page of fixed bugs has no row and is not on `notRows` (the
pages below a page of fixed bugs, such as "Expression type system", count
as such pages). A table row or list entry under a `###` group heading uses that heading's id as its anchor;
where two headings of a page have the same text, the second gets an
explicit id (`### Other fixes {#rwe-other-fixes}`).

## The pages of fixed library bugs

The fixed bugs of the libraries have one page per library, in
`src/content/docs/docs/compared-with-original/libraries/`:
`commands/NAME.md` for a command library of acidlibs, `userlibs/NAME.md`
for an RWE userlib, `amigaos.md` for the calls of AmigaOS functions. A fix
that concerns several libraries goes to the page of the first of them; a
fault shared by many libraries is described once on the section's
`index.md`, under "Fixes in several libraries". A library page begins
with the directive `::fixed-bugs-library` and ends with
`::fixed-bugs-library{part="elsewhere"}`; the section indexes list the
pages with `::fixed-bugs-libraries{group="commands"}` (or `userlibs`).
The site's remark plugin (`src/plugins/remark-bb2.mjs`) writes their
content.

The links between these pages and the command reference are made from
`data/fixoverview.yaml`, not written by hand (`src/lib/fixed-bugs.mjs`):

* The head of a library page counts its fixes by level (the rows whose
  `page` is that page) and links the library's page in the command
  reference; the end lists the rows of other pages whose `library` names
  this library.
* On these pages, a link to an issue is followed by the badge of its
  row's level and by links to the row's commands in the command
  reference. A command links to its heading when the library declares a
  command of that name; a suffix in parentheses, as in
  `PlayTracker (effect E8x)`, is ignored.
* A page of the command reference shows under each command the rows that
  name it, each linked to its entry, and the other rows of the library
  in a section "Fixed original bugs" at its end. A row of the page
  `libraries/amigaos` belongs to the deflibs library it names, every
  other row to the command library or userlib of that name.

So a row's `library` and `command` must use the names of the command
reference for the links to appear.

## API reference

`public/api/` holds the Doxygen HTML of the library headers and sources
and of `shared/` when the Pages workflow builds the site: a job runs
Doxygen 1.16.1 in its official image with the repository's `Doxyfile`
(warnings are errors) before the site's build, and Astro copies the
tree as it is (its `public/` directory) to `/bb2-c/api/`; the search
leaves it out. It is not committed (`.gitignore`). To build it locally,
from the repository root:

```
mkdir -p site/public/api
podman run --rm -v $PWD:/src -w /src ghcr.io/doxygen/doxygen:Release_1_16_1 Doxyfile
```

## Writing pages

* One directory per chapter in `src/content/docs/docs/`, each with an
  `index.md`; the order in the sidebar comes from `sidebar.order` in the
  front matter. A page is `.md`; it is `.mdx` only when it uses a
  component. A page with its images is a directory with `index.md` and
  the images next to it.
* Front matter: `title`, `description` (for cards and search),
  `sidebar.order`; the site's own fields are `aliases`, `anchorMoves`
  (above) and `width: wide` (the page uses a wider column).
* Texts are plain English, explain instead of listing, and give examples
  instead of line numbers. No repository file names in page texts.
* The original documentation (the Blitz Basic 2 manual, Acid's and Red
  When Excited's library guides) is GPL and may be used: only where it is
  correct for the port, corrected or reworded where a clearer or correct
  wording exists, with the source named on the page.
* Internal links: a site path without the base, ending in `/`, also for a
  sibling page and with an anchor:
  `[text](/docs/chapter/page/)`, `[text](/docs/chapter/page/#heading)`.
  The same in a card (`<LinkCard href="/docs/libraries/" ...>`), in `prev`
  and `next`, and in the data files. The build adds the base `/bb2-c/`;
  never write it. A relative link resolves against the URL of the page it
  is on, so it breaks easily; do not use one.
* External links: always the full URL.
* Asides for notes and warnings:

  ```
  :::note
  Text of the note.
  :::
  ```

  Types are `note`, `tip`, `caution` and `danger`; a title goes in
  brackets (`:::caution[Before you start]`).
* Components, in an `.mdx` page, imported from
  `@astrojs/starlight/components`: `CardGrid` with `LinkCard` (the
  chapter indexes and the home page), `Steps` (a numbered list of steps),
  `FileTree` (a list of files), `Tabs`/`TabItem`. A diagram is a
  ` ```mermaid ` code block.
* The site's directives, on their own line:
  `::fixed-bugs-library` (with `{part="elsewhere"}`),
  `::fixed-bugs-libraries{group="..."}` and `::library-list{what="..."}`
  (above). A directive the plugin does not know is shown as text.
* Raw HTML: Astro's Markdown has no switch for it; HTML in a page is
  written to the output as it is, in `.md` and `.mdx` alike. Use it only
  where Markdown and Starlight have no form: `<details>` with
  `<summary>` for a part the reader opens, and a `<div class="...">`
  around a block that a style of the site's CSS needs
  (`fixed-bugs-breakcode`). Everything else is Markdown, an aside, a
  component or a directive.
* Heading ids: Starlight makes them from the text (as GitHub does); an
  explicit id is written `## Heading {#id}` (in `.mdx` escaped:
  `\{#id\}`). A heading that a link or an overview row names keeps its id
  when its text changes: give it the old id explicitly.
* Old URLs: `tests/urls/hugo.txt` lists every URL the site served when
  it moved to Starlight (pages and redirect pages); the build fails for
  one that is not in its output. A page that moves gets an alias for its
  old URL (its line stays); a page removed on purpose deletes its line in
  the same commit.
* No syntax of the former Hugo site: a shortcode or `relref` (`{{<`,
  `{{%`) outside a code block fails the build.

## The checks of the build and their tests

Every check runs in `npm run build` (`src/integrations/site-checks.mjs`);
a failed check names the page or file, removes `dist/` and fails the
build. Each has a ctest with broken input (label `site`; the fixtures in
`tests/starlight/CASE/` are copied over a copy of the site):

| Check | Fails when | Tests |
| --- | --- | --- |
| warnings | npm, Astro or Vite print a warning | `site_starlight`, `site_starlight_warning` |
| leftover syntax | a shortcode or `relref` outside a code block in a page or a data file | `site_hugo_syntax` |
| overview of fixed bugs | see above | `site_fixoverview`, `site_fixoverview_*` (the data files of `tests/fixoverview/data/`, named by `BB2_FIXOVERVIEW_DATA`) |
| internal links | a link of a page or a data file leads to no page or to an id the page does not have | `site_links_missing_page`, `site_links_missing_anchor`, `site_links_data` |
| redirect pages | an `anchorMoves` names a page that is not built, an alias is a page or the alias of two pages | `site_alias_bad_move` |
| old URLs | a URL of `tests/urls/*.txt` is not in the output | `site_urls_missing` |

The API reference has its own tests, `api_doxygen` (no warning) and
`api_doxygen_broken` (a bad `@param` fails it), skipped without podman or
the Doxygen image; the examples below theirs.

## Blitz Basic examples

Every fenced ` ```blitzbasic ` block, in the pages (`src/content/`) and
in the data of the library pages (`data/`), is compiled by the tests, and
run where it can be. It is highlighted with the site's Blitz Basic
grammar (`src/lib/grammar.mjs`: the commands, objects and AmigaOS calls
of `data/libraries`, the keywords of `data/compiler/blitz2.json`); a
` ```basic ` block (a fragment that does not compile alone) is
highlighted the same way and not checked. The manifest
`tests/site-examples/examples.txt` lists each block once, by its file
and its number in that file (counted from 1): `run` (compiled and run,
the output compared with `tests/site-examples/expect/GROUP/NAME.out`) or
`compile` (compiled only, with the reason why it is not run). A block
without an entry, an entry without its block (the example went away or
changed its number, as when a block is inserted before it) or a block
listed twice fails the CMake build, so add, renumber or remove the
entries together with the blocks. A code block in another language, or
one without a language, is not checked. To check the manifest, from the
repository root, with the host tool built by the CMake build:

```
build/amiga/host/tools/siteexamples/siteexamples check site tests/site-examples/examples.txt
```

The format of the manifest and the expected output of a new `run`
example: `tests/README.md`, "Site examples".
