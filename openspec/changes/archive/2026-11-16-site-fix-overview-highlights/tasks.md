## 1. Data and layout

- [x] 1.1 Mark the six rows of the list "Changes for existing programs" (#9, #324, #391, #421, #951, #1046) with `highlight: true`
- [x] 1.2 Shortcode: the warning box lists the highlighted rows and points to the table with the count of the rows of levels 1 and 2
- [x] 1.3 Fixed-bugs index page: refer to the warning box and to the table's levels 1 and 2 separately

## 2. Check

- [x] 2.1 Check: `highlight`, if present, is a boolean; broken data file `bad-highlight` and the test `site_fixoverview_bad_highlight`
- [x] 2.2 `site/README.md`: the field `highlight` and what the warning box shows

## 3. Verification

- [x] 3.1 Build the site (no errors, no warnings), check the links, look at the overview page and the fixed-bugs index; `ctest -L site`
