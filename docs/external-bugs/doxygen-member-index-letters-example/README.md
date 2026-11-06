# Doxygen member index: letter links with a member starting with "_"

Run Doxygen in this directory:

    doxygen
    # or with the official image:
    docker run --rm -v "$PWD:/w" -w /w ghcr.io/doxygen/doxygen:Release_1_18_0 Doxyfile

`src/members.c` declares `_under` and 240 other functions, enough for a
multi-page file member index.

Check the alphabetical list at the top of `out/html/globals.html`:

    grep -o "href=\"globals[a-z_]*\.html[^\"]*\"" out/html/globals*.html | sort -u

Actual (1.17.0, 1.18.0), in `globals.html`:

    href="globals__.html#index__"     (_ : file not written)
    href="globals.html#index_a"       (a : the entries are on globals_a.html)
    href="globals_b.html#index_b"

`globals_func.html` has the same fault (`globals_func__.html`).

Expected: `_` links to the page that lists `_under` (`globals.html`), `a`
to `globals_a.html`, `b` to `globals_b.html`, each with an anchor that
exists on that page. Doxygen 1.16.1 and earlier have no such in-page
list, so the fault starts with 1.17.0.
