# Doxygen: member index letters link to missing pages (draft report)

Draft of a bug report for github.com/doxygen/doxygen, not filed yet. It
is the reason the API reference uses Doxygen 1.16.1 (`Doxyfile`,
`.github/workflows/pages.yml`); once a fixed release exists, the pin can
move to it.

Title:

    Alphabetical list on member index pages links to non-existent pages and wrong anchors when a member starts with "_" (regression in 1.17.0)

Body (follows .github/ISSUE_TEMPLATE/report-a-bug-or-issue.md):

---

**Describe the bug**

Since 1.17.0 the member index pages (`globals*.html`, and by the same code
`functions*.html`, `namespacemembers*.html`, `module*members*.html`) carry an
in-page alphabetical list (`<div class="qindex">`, added for #12008). When
at least one member starts with a character that is not a letter, e.g.
`_`, the links of that list are wrong:

1. With a multi-page index (more than `MAX_ITEMS_BEFORE_MULTIPAGE_INDEX`
   members) the links point to files Doxygen never writes
   (`globals__.html`, `globals_func__.html`, `globals_vars__.html`,
   `globals_defs__.html`), and the link of the first letter points to the
   wrong page (`A` -> `globals.html#index_a` although the `a` entries are
   on `globals_a.html`; `globals.html` holds the `_` entries).
2. On every page (also a single-page index) the anchor for `_` is
   `#index__`, but the id written on the page is `index__5F`, so the link
   does not scroll to the section.

Cause, as far as I can tell from `src/index.cpp`: `writeMemberIndex()`
(new in 1.17.0) sorts the letters by their upper-case form
(`StringMap usedLetters`, keyed by `convertUTF8ToUpper(letter)`), so the
order is `A`, `B`, `_` (`'_'` = 0x5F sorts after `'Z'`), while the pages
are written in the order of the `MemberIndexMap` (`_`, `a`, `b`, since
`'_'` sorts before `'a'`). Both loops assume that the *first* letter is
the one on the base page (`globals.html`) and all others on
`globals_<label>.html`, so the two disagree whenever a non-letter is
present. Independently of that, the anchor is built as
`anchor + letterToLabel(letter)` without `convertToId()`, whereas the page
writes `"index_" + convertToId(letterToLabel(letter))` (`index__5F` for
`_`). The older `writeQuickMemberIndex()` (used for
`HTML_DYNAMIC_MENUS = NO`) keeps the map order and uses `convertToId()`,
and its output is consistent. The navigation tree data (`globals_dup.js`)
is also correct; only the in-page letter list is wrong.

**Screenshots**

(none; the HTML snippets below show the problem)

**To Reproduce**

Self-contained example: one C file with one global starting with `_` and
240 other globals so that the index is split into one page per letter.

```sh
mkdir -p repro/src && cd repro
{ printf '/** @file */\n/** u */ void _under(void);\n'
  for i in $(seq 1 120); do printf 'void alpha%d(void);\nvoid beta%d(void);\n' $i $i; done
} > src/t.c
cat > Doxyfile <<'EOF'
INPUT = src
OUTPUT_DIRECTORY = out
GENERATE_LATEX = NO
QUIET = YES
EXTRACT_ALL = YES
WARN_IF_UNDOCUMENTED = NO
EOF
doxygen Doxyfile
ls out/html/globals*.html
grep -o 'href="globals[^"]*#index[^"]*"[^>]*>[^<]*</a>' out/html/globals.html
grep -o 'id="index_[^"]*"' out/html/globals.html out/html/globals_a.html
```

Output with 1.18.0 (also with 1.17.0):

```
globals.html  globals_a.html  globals_b.html  globals_func.html  globals_func_a.html  globals_func_b.html

href="globals.html#index_a">A</a>
href="globals_b.html#index_b">B</a>
href="globals__.html#index__">_</a>

out/html/globals.html:id="index__5F"
out/html/globals_a.html:id="index_a"
```

So `A` links to `globals.html`, which only contains `_under`; `_` links to
`globals__.html`, which does not exist, with an anchor (`index__`) that
exists nowhere (the page uses `index__5F`). The same happens on
`globals_a.html`, `globals_b.html` and the three `globals_func*.html`
pages. With 20 instead of 120 iterations (single page) the file links are
fine but the `_` link is still `#index__` while the id is `index__5F`.

(I can attach the example as a zip if preferred; the script above
reproduces it exactly.)

**Expected behavior**

The alphabetical list links to the pages and anchors Doxygen actually
writes, as the navigation tree (`globals_dup.js`) does:

```
href="globals.html#index__5F">_</a>   (or globals__.html if the `_` page is named so)
href="globals_a.html#index_a">A</a>
href="globals_b.html#index_b">B</a>
```

i.e. the letter list and the page writer must agree on which letter is
on the base page, and the anchor must be the same `convertToId()` form the
page uses. 1.16.1 and earlier produce no broken links for this example.

**Version**

- 1.18.0 (official image `ghcr.io/doxygen/doxygen:Release_1_18_0`,
  Ubuntu 22.04 inside the image), run with podman on CachyOS Linux
  (Arch-based), x86_64.
- Also reproduced with 1.17.0 (Alpine 3.24 package).
- Not reproducible with 1.16.1, 1.16.0, 1.15.0, 1.14.0, 1.13.2, 1.13.0,
  1.12.0 (official images) and 1.9.8 (Debian trixie package): no links to
  missing files, no in-page letter list with wrong anchors.
- The code on `master` (`src/index.cpp`, `writeMemberIndex()`) is
  unchanged, so the problem should still be present there.

First bad commit (by reading the history of `src/index.cpp`; not built
and bisected locally): d6ea6c14708a6a84db2a008cc89a2c4ff92e8d27
("issue #12008 ... This has been adjusted (and labels and filenames are
left unchanged)", 2026-03-10) introduced the upper-case sort order that
breaks the "first letter is the base page" assumption. The missing
`convertToId()` in the anchor goes back to the commit that added
`writeMemberIndex()` (c74988f74a, 2026-02-25, PR #12016).

**Stack trace**

Not applicable (no crash).

**Additional context**

Workaround: `HTML_DYNAMIC_MENUS = NO` (the in-page list is then not
written; navigation goes through the static menu, which is correct).
Any real-world C project with identifiers starting with `_` and more than
200 globals is affected; the broken links show up in link checkers.
