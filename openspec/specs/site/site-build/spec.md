# site/site-build Specification

## Purpose

Defines how the project website in `site/` is built: with pinned generator
versions, without warnings, with its internal links and old URLs checked,
so that a published site has no broken link and every bookmarked URL
still leads to its content.

## Requirements

### Requirement: Pinned generator versions
The site's dependencies SHALL be pinned to exact versions, with a
committed lock file, and installed only from that lock file; Node.js SHALL
be pinned to one version: the Pages workflow SHALL run it in the container
image pinned by that version's tag, the tests and the local build either
in that image or with the host's Node.js when its version is the pinned
one. No build
SHALL update a dependency; an update SHALL be a commit of its own.

#### Scenario: A clean install
- **WHEN** the site's dependencies are installed in a fresh checkout
- **THEN** exactly the versions of the committed lock file are installed, and the lock file is unchanged

#### Scenario: A newer release exists
- **WHEN** a newer release of the generator exists and nobody updated the pins
- **THEN** the Pages workflow builds with the pinned versions

### Requirement: The site builds without warnings
The site build SHALL finish without errors and without warnings. The test
that builds the site as it is SHALL fail when the build prints a warning.

#### Scenario: A warning
- **WHEN** the build prints a warning (as for an empty collection or a large chunk)
- **THEN** the test of the unbroken site fails and shows the warning

### Requirement: A failed check leaves no site
When a check of the site build fails, the build SHALL fail with a message
that names the problem, and SHALL leave no built site behind.

#### Scenario: A failed check
- **WHEN** a check of the build fails
- **THEN** the build exits with an error and the output directory holds no built site

### Requirement: Internal links are checked
A link of a page or of the library data to a page of the site that does
not exist, or to a heading id that the target page does not have, SHALL
fail the site build with a message naming the page and the link. Links
SHALL be written as site paths (`/docs/...`) or relative to the page;
the build SHALL add the base path.

#### Scenario: A link to a missing page
- **WHEN** a page links `/docs/no-such-page/`
- **THEN** the build fails and names the page and the link

#### Scenario: A link to a missing heading
- **WHEN** a page links `/docs/libraries/commands/windowslib/#no-such-heading`
- **THEN** the build fails and names the page and the link

#### Scenario: Built for GitHub Pages
- **WHEN** a page links `/docs/libraries/` and the site is built for `https://fubar-coder.github.io/bb2-c/`
- **THEN** the link in the output is `/bb2-c/docs/libraries/`

### Requirement: No syntax of the former generator
A shortcode or `relref` of the former generator (`{{<`, `{{%`) outside a
code block in a page or in the library data SHALL fail the site build,
with a message naming the file.

#### Scenario: A leftover relref
- **WHEN** a data file of `site/data/cmddesc` contains `{{< relref "/docs/libraries" >}}` outside a code block
- **THEN** the build fails and names the file

### Requirement: Every old URL is still served
A committed list SHALL hold every URL the site served before the move to
the current generator (pages and redirect pages). The site build SHALL
fail, naming the URL, when one of them is not in the output as a page or a
redirect page.

#### Scenario: A page without its URL
- **WHEN** a page of the list is renamed without a redirect for its old URL
- **THEN** the build fails and names the old URL

### Requirement: Blitz Basic code is highlighted from the library data
A fenced code block with the language `blitzbasic` SHALL be highlighted
with a Blitz Basic grammar whose command, object and AmigaOS call names
come from the committed command reference data, so that a command added
to a library is highlighted without a change to the grammar.

#### Scenario: A library command
- **WHEN** a `blitzbasic` block contains `NPrint "a"` and `site/data/libraries` declares `NPrint`
- **THEN** `NPrint` is shown as a command and `"a"` as a string

#### Scenario: Another language
- **WHEN** a block has no language or another language
- **THEN** it is not highlighted as Blitz Basic

### Requirement: The site is tested outside the CMake build
The site SHALL NOT be a target of the CMake build. The ctests of the
label `site` SHALL build the site with the host's Node.js when its version
is the pinned one, else in the pinned container image, and SHALL be
skipped (not failed) when neither way or the dependencies are not
available. Either way SHALL be selectable for the tests; a selected way
that is not available SHALL fail the tests.

#### Scenario: The host's Node.js
- **WHEN** `ctest -L site` runs on a machine whose Node.js has the pinned version
- **THEN** the site is built with the host's Node.js, without podman

#### Scenario: No Node.js at the pinned version
- **WHEN** `ctest -L site` runs on a machine whose Node.js is missing or of another version, and without podman or without the pinned image
- **THEN** the site tests are reported as skipped

#### Scenario: A selected way is not available
- **WHEN** the tests are configured for the container and podman or the image is missing, or for the host and its Node.js is missing or of another version
- **THEN** the site tests fail with a message naming what is missing
