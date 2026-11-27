# Spec Delta

## MODIFIED Requirements

### Requirement: Built without warnings in the Pages workflow
The Pages workflow SHALL run Doxygen 1.16.1 in its official container
image, pinned by tag, with warnings treated as errors, into
`site/public/api/` before the site is built, so that the reference is
served under `api/` of the website. The generated tree SHALL NOT be
committed. A comment that makes Doxygen warn SHALL be corrected in the
comment; no result of the build SHALL change by it.

#### Scenario: A broken comment
- **WHEN** a routine's comment documents a parameter the routine does not have
- **THEN** the Doxygen run fails and the site is not published

#### Scenario: Comment fixes
- **WHEN** the comments are corrected
- **THEN** the programs, acidlibs, deflibs and every library file are byte-identical to the build before

#### Scenario: Served beside the site
- **WHEN** the Pages workflow has built the site with the API reference
- **THEN** `/bb2-c/api/index.html` is the Doxygen main page and the site's search does not index it
