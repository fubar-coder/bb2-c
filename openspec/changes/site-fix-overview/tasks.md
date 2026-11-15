## 1. Decide the open points

- [x] 1.1 Settle the open questions of design.md with the user (title, ordering of severity 1, topic list, rows per issue, library column for TED/Blitz 2) - done (2026-11-15): design.md, "Decisions on the open questions (user, 2026-11-15)"

## 2. Data and classification

- [x] 2.1 Define the data file `site/data/fixoverview.yaml`: `levels`, `topics`, `notRows`, `rows` with the fields of design.md, and document the fields in `site/README.md`
- [x] 2.2 Classify all issues linked on the fixed-bugs pages (about 250 rows) by an agent: severity, topic, library or command, one sentence, page and anchor; proposal in `~/.local/share/bb2-c-work/` - done (2026-11-15): 371 rows; proposal `~/.local/share/bb2-c-work/site-fix-overview-classification.yaml`, levels of the round reports kept where given
- [ ] 2.3 Show the severity 1 rows to the user and apply the corrections
- [x] 2.4 Add explicit heading ids on the fixed-bugs pages where a group heading's generated id is ambiguous - done: `#other-strings`, `#amigaos-other-fixes`, `#rwe-other-fixes` (libraries), `#other-joining-lines` (editor)

## 3. Page, shortcode and script

- [x] 3.1 Write the shortcode that renders the table in the initial order with `th scope`, `relref` links and data attributes
- [x] 3.2 Write `site/assets/js/fix-overview.js` (sort buttons with `aria-sort`, severity and topic filter with labels, live count) and the style
- [x] 3.3 Write the page with the explanation of the four levels; link it from the chapter `_index.md` and from the fixed-bugs pages
- [x] 3.4 Build the site (podman, `site/README.md`), look at the page in light and dark mode with and without JavaScript, test keyboard use; build without errors or warnings

## 4. The check

- [x] 4.1 Write the partial with the checks of design.md Decision 6 (anchor, severity, topic, duplicates, entry without row, `notRows`) failing with `errorf`
- [x] 4.2 Fill `notRows` with the cross-references found by the first run, each with a reason - done: the first run found no cross-reference (every linked issue is a fix), the list is empty
- [x] 4.3 Add the data file parameter and the broken data files under `site/tests/` (missing anchor, severity 5, unknown topic, duplicate issue, entry without row) - done: `site/tests/fixoverview/` (data files and the config that mounts them), the parameter `fixOverviewData` set through `HUGO_PARAMS_FIXOVERVIEWDATA`
- [x] 4.4 Add the CMake test (label `site`, 77 without podman or image) that expects each broken file to fail with its message and the real one to build - done: `site/tests/CMakeLists.txt`, `cmake/site-fixoverview-test.cmake`; `site_fixoverview` and five `site_fixoverview_*`

## 5. Specs and docs

- [x] 5.1 Check `site/fix-overview` and the `testing/bug-fix-workflow` delta against the implementation; adjust the text where the decisions of 1.1 changed it - done: initial order by severity and issue number with the severity-1 warning box (decision 2), the header "Library or command", the unknown-topic case of the test
- [x] 5.2 State the new website step in `docs/development.md` or `tests/README.md` where the fix workflow is described, and in `.claude/agents/implementer.md` (report proposes severity and topic) - done: `docs/development.md` (fix step 3), `.claude/agents/implementer.md`

## 6. CLAUDE.md

- [ ] 6.1 Propose the sentence for CLAUDE.md fix workflow step 3 ("one row per fixed issue in the overview data file") to the user; edit CLAUDE.md only with the user's OK

## 7. Closing

- [x] 7.1 Run `openspec validate site-fix-overview`, the site build and the new test; commit one topic per commit - done (2026-11-15): valid; site build clean; `ctest -L site` 6/6
