---
# The fixture of site_alias_bad_move: an anchorMoves entry that names a
# page that does not exist fails the build (src/lib/redirects.mjs),
# naming this page and the page it names.
title: Fixture alias
aliases: [/fixture-old-url/]
anchorMoves:
  some-entry: no-such-page
---

A page with an old URL.
