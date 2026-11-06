# bb2-c website

Source of the project website, <https://fubar-coder.github.io/bb2-c/>. It is
a [Hugo](https://gohugo.io/) site (extended edition) with the
[Hextra](https://imfing.github.io/hextra/) theme as a Hugo module. It is not
part of the CMake build; GitHub Actions builds and publishes it
(`.github/workflows/pages.yml`) on every push to `main` that touches `site/`.

The site is written for readers of the project, not for browsing the
repository. `docs/` stays the internal developer documentation and is the
source of facts, but is not included here; the pages are new texts.

## Local preview

```
podman run --rm -p 1313:1313 -v $PWD/site:/src hugomods/hugo:exts \
    hugo server --bind 0.0.0.0 --poll 700ms --enableGitInfo=false
```

`--enableGitInfo=false` is needed because only `site/` is mounted and the
container then sees no repository (the CI build has one). To keep the
"last modified" dates in the preview, mount the repository root and add
`--source site`. Add `:Z` to the volume on SELinux hosts.

A one-off build, as CI does it, into `site/public/`:

```
podman run --rm -v $PWD/site:/src hugomods/hugo:exts \
    hugo --gc --minify --enableGitInfo=false --baseURL https://fubar-coder.github.io/bb2-c/
```

The build must finish without errors or warnings.

## Theme

Hextra is pinned in `go.mod` and `go.sum`. CI never updates it. To move to a
newer release, run `hugo mod get github.com/imfing/hextra@vX.Y.Z` and
`hugo mod tidy` in `site/` and commit the result as its own commit.

## Writing pages

* One directory per chapter in `content/docs/`, each with an `_index.md`; the
  order in the sidebar comes from `weight` in the front matter.
* Texts are plain English, explain instead of listing, and give examples
  instead of line numbers. No repository file names in page texts.
* Nothing from the CD or the original manual.
* Internal links: always `{{< relref "/docs/chapter/page" >}}` in a link
  target (`[text]({{< relref "/docs/libraries" >}})`), also for a sibling page
  and with an anchor (`"/docs/chapter/page#heading"`). A relative link
  (`[text](page)`) is left as it is and resolves against the URL of the page
  it is on: on a page `/docs/chapter/page/` it points below that page and
  breaks. `relref` also fails the build when the target does not exist.
  Only the `link` of a `card` in a chapter's `_index.md` may be the plain
  name of a page of that chapter, since the chapter's URL is its directory.
* External links: always the full URL, including the menu entries in
  `hugo.yaml`. The site lives under the `/bb2-c/` subpath, and Hugo prefixes
  it to menu URLs that begin with `/`.
* The navbar search needs the menu entry with `params.type: search` in
  `hugo.yaml`; do not remove it.
* Notes and warnings use Hextra callouts:

  ```
  {{< callout type="warning" >}}
  Text of the warning.
  {{< /callout >}}
  ```

  Types are `info`, `warning`, `error` and `important`.
* Raw HTML in Markdown is switched off; use shortcodes.
