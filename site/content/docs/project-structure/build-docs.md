---
title: Building the documentation
weight: 5
description: How to build this website and its API reference locally, update the command reference after a library change, and how the Pages workflow publishes it.
next: /docs
---

Before this page you should have a clone of the repository and Podman (or
another container tool) on your machine.

This website has three parts, made by three tools: the pages, written by
hand and built by Hugo; the command reference, generated from the built
library files; and the API reference for C developers, made by Doxygen
from the comments in the sources. Everything runs in containers with
pinned versions, so your local result is the same as the published one.
All commands on this page are run from the repository's root.

## The API reference

Doxygen reads the library headers, the sources of the libraries of Blitz
Basic 2 and of the RWE userlibs, and the shared code, with the
configuration `Doxyfile` in the repository's root. It writes HTML into
`site/static/api/`, which Hugo then publishes as it is under `/api/` of
the site:

```
mkdir -p site/static/api
podman run --rm -v $PWD:/src -w /src ghcr.io/doxygen/doxygen:Release_1_16_1 Doxyfile
```

The configuration treats a comment error, such as a `@param` for a
parameter that does not exist, as an error: the run fails, and so does
the publication of the site. The directory is not committed; build it
again after changing comments. How the comments are written is described
for the userlib tutorial
([Documenting the library]({{< relref "/docs/blitz2-internals/own-userlib#documenting-the-library" >}})).

## The site

The site is a Hugo project in `site/`, with the theme Hextra pinned as a
Hugo module. It is built with the same Hugo image as in the Pages
workflow. For a preview that rebuilds on every change:

```
podman run --rm -p 1313:1313 -v $PWD:/src -w /src docker.io/hugomods/hugo:exts-0.154.5 \
    hugo server --source site --bind 0.0.0.0 --poll 700ms
```

and open `http://localhost:1313/bb2-c/`. The repository's root is
mounted, not only `site/`, because the pages show the date of their last
change, which Hugo takes from Git. Build the API reference first if you
want its links to work in the preview.

A one-off build, as the Pages workflow does it, writes the site into
`site/public/`:

```
podman run --rm -v $PWD:/src -w /src docker.io/hugomods/hugo:exts-0.154.5 \
    hugo --gc --minify --source site --baseURL https://fubar-coder.github.io/bb2-c/
```

It must finish without errors and warnings. `site/public/` is not
committed. The conventions for writing pages are in the
[README of the site](https://github.com/fubar-coder/bb2-c/blob/main/site/README.md).

{{< callout type="info" >}}
In a Git worktree, the `.git` of the checkout is a file that points into
the main repository's `.git` directory. Mount that directory at its own
path as well (`-v /path/to/repo/.git:/path/to/repo/.git:ro`), or Hugo
cannot read the dates of the pages.
{{< /callout >}}

## The command reference

The pages of the
[command reference]({{< relref "/docs/libraries" >}}) are made by Hugo
from one data file per library in `site/data/libraries/`, which the host
tool `blitzlib cmdref` generates from the built library files. These files are
committed, and the build of the port checks, in the build step of each
library, that the committed file equals the one it generates from the
library just built.

So when you change a library's commands on purpose, its help text, its
arguments or a new command, the build stops with a message that names
both files and says what to do. Then copy the generated files into the
site and commit them with your change:

```
cmake --build --preset amiga --target cmdref_update
```

Do not edit these files by hand. A description and an introduction of a
library, written by hand, go into `site/data/libintros/`; the README of
the site describes their format.

## How the Pages workflow publishes it

The workflow `.github/workflows/pages.yml` runs on every push to `main`
that changes the site, the Doxyfile or the sources the API reference
reads, and it can be started by hand. It has three jobs:

1. **api** runs Doxygen in the same image as above and keeps
   `site/static/api/` as an artifact. A comment error ends the workflow
   here.
2. **build** runs in the Hugo image above, with the full Git history for
   the dates of the pages, puts the API reference back into
   `site/static/api/`, and builds the site with the address GitHub Pages
   gives it.
3. **deploy** publishes the result on GitHub Pages.

The command reference needs no job of its own: its data files are
committed, and the build of the port has already checked them.
