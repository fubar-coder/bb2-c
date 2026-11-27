# Spec Delta

## MODIFIED Requirements

### Requirement: Links keep the base path
Links of the page to other pages of the site SHALL be written as site
paths (`/docs/...`), to which the site build adds the base, so that they
work under the `/bb2-c/` base; the issue links point to GitHub.

#### Scenario: Built for GitHub Pages
- **WHEN** the site is built with the base URL `https://fubar-coder.github.io/bb2-c/`
- **THEN** every detail link starts with `/bb2-c/docs/compared-with-original/`

### Requirement: The old URLs still work
A page of fixed bugs that moved, and the removed sections `fixed-bugs/` and `new-errors/`, SHALL keep their old URLs as `aliases` in the front matter of the page that now holds their content. The site build SHALL write a page at such a URL that forwards to the page and keeps the anchor of the link.

#### Scenario: An old link with an anchor
- **WHEN** a reader opens `/docs/compared-with-original/fixed-bugs/compiler/#arrays-256`
- **THEN** the page at that URL forwards to `/docs/compared-with-original/language/#arrays-256`

#### Scenario: Without JavaScript
- **WHEN** a reader opens an old URL with JavaScript off
- **THEN** the page forwards to the page that holds the content (without the anchor) and links it
