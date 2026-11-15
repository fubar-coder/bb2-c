## 1. Decide the open points

- [ ] 1.1 Settle the open questions of design.md with the user (title, ordering of severity 1, topic list, rows per issue, library column for TED/Blitz 2)

## 2. Data and classification

- [ ] 2.1 Define the data file `site/data/fixoverview.yaml`: `levels`, `topics`, `notRows`, `rows` with the fields of design.md, and document the fields in `site/README.md`
- [ ] 2.2 Classify all issues linked on the fixed-bugs pages (about 250 rows) by an agent: severity, topic, library or command, one sentence, page and anchor; proposal in `~/.local/share/bb2-c-work/`
- [ ] 2.3 Show the severity 1 rows to the user and apply the corrections
- [ ] 2.4 Add explicit heading ids on the fixed-bugs pages where a group heading's generated id is ambiguous

## 3. Page, shortcode and script

- [ ] 3.1 Write the shortcode that renders the table in the initial order with `th scope`, `relref` links and data attributes
- [ ] 3.2 Write `site/assets/js/fix-overview.js` (sort buttons with `aria-sort`, severity and topic filter with labels, live count) and the style
- [ ] 3.3 Write the page with the explanation of the four levels; link it from the chapter `_index.md` and from the fixed-bugs pages
- [ ] 3.4 Build the site (podman, `site/README.md`), look at the page in light and dark mode with and without JavaScript, test keyboard use; build without errors or warnings

## 4. The check

- [ ] 4.1 Write the partial with the checks of design.md Decision 6 (anchor, severity, topic, duplicates, entry without row, `notRows`) failing with `errorf`
- [ ] 4.2 Fill `notRows` with the cross-references found by the first run, each with a reason
- [ ] 4.3 Add the data file parameter and the broken data files under `site/tests/` (missing anchor, severity 5, unknown topic, duplicate issue, entry without row)
- [ ] 4.4 Add the CMake test (label `site`, 77 without podman or image) that expects each broken file to fail with its message and the real one to build

## 5. Specs and docs

- [ ] 5.1 Check `site/fix-overview` and the `testing/bug-fix-workflow` delta against the implementation; adjust the text where the decisions of 1.1 changed it
- [ ] 5.2 State the new website step in `docs/development.md` or `tests/README.md` where the fix workflow is described, and in `.claude/agents/implementer.md` (report proposes severity and topic)

## 6. CLAUDE.md

- [ ] 6.1 Propose the sentence for CLAUDE.md fix workflow step 3 ("one row per fixed issue in the overview data file") to the user; edit CLAUDE.md only with the user's OK

## 7. Closing

- [ ] 7.1 Run `openspec validate site-fix-overview`, the site build and the new test; commit one topic per commit
