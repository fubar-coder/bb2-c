## 1. Data

- [x] 1.1 Write the five levels with name and text in `site/data/fixoverview.yaml` and renumber the rows mechanically (2 to 4, 4 to 5)
- [x] 1.2 Sort the 20 old level-1 rows into 1 or 2 by their fixed-bugs sections and issues, and move #1067 to 2 - done (2026-11-16): 13 rows stay at 1 (#9, #48, #96, #212, #254, #255, #277, #324, #391, #421, #1046, #1055, #1069), #97, #98, #103, #292, #408, #951, #985 and #1067 at 2

## 2. Layout and check

- [x] 2.1 Shortcode: the warning box lists levels 1 and 2 in two lists; the stylesheet colours the badges of levels 1 and 2
- [x] 2.2 Check: the levels must be 1 to 5 in order with name and text; broken data file `bad-levels`, `bad-severity` uses 6; the test data have five levels
- [x] 2.3 `site/README.md`: the levels and the check

## 3. Pages

- [x] 3.1 `important-fixes.md`: five levels, the ranking of runtime above compile errors, levels 3 and 4 on "Sources that compile differently"
- [x] 3.2 Libraries page: "Changes for existing programs" with the level-1 sections, "Changes you may notice" with the level-2 ones; the fixed-bugs index without a hand count
- [x] 3.3 Build the site (no errors, no warnings), check the links, look at the overview page and the warning box

## 4. Rules

- [x] 4.1 `.claude/agents/implementer.md`: severity 1-5
