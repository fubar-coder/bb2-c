---
title: Building the documentation
description: How to build this website and its API reference locally, update the
  command reference after a library change, and how the Pages workflow publishes
  it.
sidebar:
  order: 8
next:
  link: /docs/
  label: Documentation
---

Before this page you should have a clone of the repository, and either
Node.js in the version the site pins or Podman (or another container
tool) on your machine.

This website has three parts, made by three tools: the pages, written by
hand and built by Astro with its documentation theme Starlight; the
command reference, generated from the built library files; and the API
reference for C developers, made by Doxygen from the comments in the
sources. The tools run with pinned versions, so your local result is the
same as the published one. All commands on this page are run from the
repository's root unless they say otherwise.

## The API reference

Doxygen reads the library headers, the sources of the libraries of Blitz
Basic 2 and of the RWE userlibs, and the shared code, with the
configuration `Doxyfile` in the repository's root. It writes HTML into
`site/public/api/`, which the site's build then publishes as it is under
`/api/` of the site:

```
mkdir -p site/public/api
podman run --rm -v $PWD:/src -w /src ghcr.io/doxygen/doxygen:Release_1_16_1 Doxyfile
```

The configuration treats a comment error, such as a `@param` for a
parameter that does not exist, as an error: the run fails, and so does
the publication of the site. The directory is not committed; build it
again after changing comments. How the comments are written is described
for the userlib tutorial
([Documenting the library](/docs/blitz2-internals/own-userlib/#documenting-the-library)).

## The site

The site is an Astro project in `site/`. Its packages have exact
versions in `site/package.json` and `site/package-lock.json`, and it is
built with Node.js 26.13.1, the version named under `engines` in
`site/package.json`. You can use Node.js installed on your machine, if
`node --version` prints `v26.13.1`, or the official Node.js image of
that version, which the Pages workflow uses too.

With Node.js on your machine, in `site/`, install the packages once and
start the development server, which shows every change of a page at
once:

```
cd site
npm ci
npm run dev
```

and open `http://localhost:4321/bb2-c/`. The same in the container, from
the repository's root:

```
podman run --rm -v $PWD/site:/site -w /site \
    docker.io/library/node:26.13.1-bookworm-slim npm ci
podman run --rm -it -p 4321:4321 -v $PWD/site:/site -w /site \
    docker.io/library/node:26.13.1-bookworm-slim npm run dev
```

and open `http://127.0.0.1:4321/bb2-c/` (the server in the container
answers on IPv4 only). Stop the server with Ctrl-C. Build the API
reference first if you want its links to work.

A full build, as the Pages workflow does it, writes the site into
`site/dist/`; `npm run preview` then serves that result at the same
address. With Node.js on your machine, still in `site/`:

```
npm run build
npm run preview
```

or in the container, from the repository's root:

```
podman run --rm -v $PWD/site:/site -w /site \
    docker.io/library/node:26.13.1-bookworm-slim npm run build
podman run --rm -it -p 4321:4321 -v $PWD/site:/site -w /site \
    docker.io/library/node:26.13.1-bookworm-slim npm run preview
```

The build checks every link between the pages and to their headings,
the overview of fixed bugs and the addresses of moved pages, and stops
with the page and the link that is wrong. It must finish without errors
and warnings. `site/dist/` is not committed. The conventions for writing
pages are in the
[README of the site](https://github.com/fubar-coder/bb2-c/blob/main/site/README.md).

The pages show the date of their last change only to search engines, as
a tag in the page's head; it comes from Git and is written only by the
Pages workflow, so a local build needs no Git.

## The command reference

The pages of the
[command reference](/docs/libraries/) are made by the site's build
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
   `site/public/api/` as an artifact. A comment error ends the workflow
   here.
2. **build** runs in the Node.js image above, with the full Git history
   for the dates of the pages, puts the API reference back into
   `site/public/api/`, installs the packages with `npm ci` and builds the
   site.
3. **deploy** publishes the result on GitHub Pages.

The command reference needs no job of its own: its data files are
committed, and the build of the port has already checked them.
