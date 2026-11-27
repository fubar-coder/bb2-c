// The parts of the pages of fixed library bugs and of the command reference
// that come from data/fixoverview.yaml (site-starlight, task 5.1): the port
// of Hugo's partials layouts/_partials/fixed-bugs/{level,refpage,reflinks}.html,
// the shortcodes fixed-bugs-library and fixed-bugs-libraries and the link
// hook layouts/docs/compared-with-original/libraries/_markup/render-link.html.
// The site's remark plugin (src/plugins/remark-bb2.mjs) puts them on the
// pages; the command reference (src/lib/cmdref.mjs) uses the badges.
//
// Every link is a site path without the base ("/docs/a/b/#c"): in Markdown
// the remark plugin adds it, in HTML the caller passes url(path), which adds
// it.
import { readdirSync, readFileSync } from "node:fs";
import { join } from "node:path";
import YAML from "yaml";
import { SITE, byText, entryIds, fixoverview, levels, libraries, rowsByIssue } from "./data.mjs";

/** The directory of the pages of fixed bugs, as a row's "page" is relative to it */
export const FIXBASE = "/docs/compared-with-original";
/** The issue links that get the badge on the pages of fixed library bugs */
export const ISSUE = /^https:\/\/github\.com\/fubar-coder\/bb2-c\/issues\/([0-9]+)$/;

/** HTML escape of a text (Go's html/template does it on every value) */
export const esc = (s) => String(s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);

/** A list field of a row ("a, b") as its items */
export const splitList = (s) =>
  String(s ?? "")
    .split(",")
    .map((x) => x.trim())
    .filter(Boolean);

/** The command of a row's "command" item, without an explanation in parentheses */
export const commandName = (c) => c.replace(/\s*\(.*\)$/, "").trim();

/** The link of a row to its entry on the page of fixed bugs (no base) */
export const rowHref = (row) => `${FIXBASE}/${row.page}/#${row.anchor}`;

/**
 * A badge with the name of a level (layouts/_partials/fixed-bugs/level.html),
 * with count shown after the name if given. The colours: the classes
 * fixed-bug-level-N (src/styles/fixed-bugs.css), Hugo sets the theme's
 * badge classes instead.
 */
export function levelBadge(level, count) {
  const l = levels()[String(level)];
  const text = count ? `${l.name}: ${count}` : l.name;
  return `<span class="fixed-bug-level fixed-bug-level-${level}" title="Level ${level}: ${esc(l.text)}">${esc(text)}</span>`;
}

/** The badges of the rows by level, with their counts, in level order */
export const levelBadges = (rows) =>
  [1, 2, 3, 4, 5]
    .map((l) => [l, rows.filter((r) => r.severity === l).length])
    .filter(([, n]) => n)
    .map(([l, n]) => levelBadge(l, n))
    .join(" ");

/**
 * The page of the command reference for a library name of a row
 * (layouts/_partials/fixed-bugs/refpage.html; page: the row's page):
 * {path, title} or null. A row of the page of the AmigaOS calls names a
 * deflibs library (its page under amigaos/ if it opens an AmigaOS library),
 * any other row a command library or an RWE userlib.
 */
export function refpage(lib, page) {
  const L = libraries();
  if (page === "libraries/amigaos") {
    const d = L.deflibs[lib];
    return d?.opens ? { path: `/docs/libraries/amigaos/${lib}/`, title: d.opens } : null;
  }
  if (L.acidlibs[lib]) return { path: `/docs/libraries/commands/${lib}/`, title: lib };
  if (L.userlibs[lib]) return { path: `/docs/libraries/userlibs/${lib}/`, title: lib };
  return null;
}

/**
 * The links of a row to the command reference
 * (layouts/_partials/fixed-bugs/reflinks.html): each command of the row
 * that its library declares links to the command's heading, a library none
 * of whose commands is found to its page. [{href, text, title, code}]
 */
export function reflinks(row) {
  const cmds = splitList(row.command).map((c) => commandName(c).toLowerCase());
  const links = [];
  for (const lib of splitList(row.library)) {
    const ref = refpage(lib, row.page);
    if (!ref) continue;
    let found = 0;
    const data = libraries().acidlibs[lib] ?? libraries().userlibs[lib];
    if (data && row.page !== "libraries/amigaos")
      for (const { entry, id } of entryIds(data.entries))
        if (cmds.includes(entry.name.toLowerCase())) {
          links.push({ href: `${ref.path}#${id}`, text: entry.name, title: `${entry.name} in the reference of ${lib}`, code: true });
          found++;
        }
    if (!found) links.push({ href: ref.path, text: ref.title, title: `The reference of ${lib}`, code: false });
  }
  return links;
}

// Hextra's icon "book-open", as Hugo's pages have it
const BOOK =
  '<svg class="fixed-bug-book" height="1em" fill="none" viewBox="0 0 24 24" stroke-width="2" stroke="currentColor" aria-hidden="true"><path stroke-linecap="round" stroke-linejoin="round" d="M12 6.253v13m0-13C10.832 5.477 9.246 5 7.5 5S4.168 5.477 3 6.253v13C4.168 18.477 5.754 18 7.5 18s3.332.477 4.5 1.253m0-13C13.168 5.477 14.754 5 16.5 5c1.747.0 3.332.477 4.5 1.253v13C19.832 18.477 18.247 18 16.5 18c-1.746.0-3.332.477-4.5 1.253"/></svg>';

/**
 * The HTML after an issue link on a page of fixed library bugs (the link
 * hook render-link.html): the badge of the issue's level and its links to
 * the command reference; "" for an issue without a row.
 */
export function issueMeta(issue, url) {
  const row = rowsByIssue()[String(issue)];
  if (!row) return "";
  const refs = reflinks(row)
    .map((l) => `<a href="${esc(url(l.href))}" title="${esc(l.title)}">${l.code ? `<code>${esc(l.text)}</code>` : esc(l.text)}</a>`)
    .join(", ");
  return ` <span class="fixed-bug-meta">${levelBadge(row.severity)}${refs ? ` <span class="fixed-bug-refs">${BOOK}${refs}</span>` : ""}</span>`;
}

/** The key of a page of fixed bugs by its site path: "libraries/commands/windowslib" */
export const pageKey = (path) => path.replace(/^\/?docs\/compared-with-original\//, "").replace(/\/$/, "");

/**
 * The head of a page of fixed library bugs (the shortcode
 * fixed-bugs-library, its part "head"), as HTML: the number of fixes on the
 * page by level and the library's page in the command reference.
 */
export function fixedBugsHead(key, url) {
  const lib = key.split("/").pop();
  const rows = fixoverview().rows.filter((r) => r.page === key);
  const os = key === "libraries/amigaos";
  const ref = os ? { path: "/docs/libraries/amigaos/", title: "AmigaOS libraries" } : refpage(lib, key);
  const what = os ? "The functions of AmigaOS" : "The commands of the library";
  return `<p class="fixed-bugs-head">${rows.length} fixed ${rows.length === 1 ? "bug" : "bugs"} on this page, by level: ${levelBadges(rows)}${
    ref ? `<br>${what}: <a href="${esc(url(ref.path))}">${esc(ref.title)}</a> in the command reference.` : ""
  }</p>`;
}

/**
 * The part "elsewhere" of the shortcode fixed-bugs-library, as Markdown (so
 * that its heading is in the table of contents): the section "Fixes
 * described with other libraries", or "" if the library has none.
 */
export function fixedBugsElsewhere(key) {
  if (key === "libraries/amigaos") return "";
  const lib = key.split("/").pop();
  const rows = fixoverview()
    .rows.filter((r) => r.page !== key && r.page !== "libraries/amigaos" && splitList(r.library).includes(lib))
    .sort((a, b) => a.issue - b.issue);
  if (!rows.length) return "";
  return (
    `## Fixes described with other libraries\n\nThese fixes of ${lib} are described on the pages of other libraries,\nor once for several libraries.\n\n` +
    rows.map((r) => `- [${r.text}](${rowHref(r)}) <span class="fixed-bug-meta">${levelBadge(r.severity)}</span>\n`).join("")
  );
}

/** The front matter of a page file */
function frontMatter(file) {
  const m = readFileSync(file, "utf8").match(/^---\n([\s\S]*?)\n---\n/);
  return (m && YAML.parse(m[1])) || {};
}

/**
 * The table of the pages of fixed library bugs of a group (the shortcode
 * fixed-bugs-libraries; group "commands" or "userlibs"), as HTML: each page
 * (the converted pages of src/content/docs, by title) with its number of
 * fixes, the badges by level and its page in the command reference.
 */
export function fixedBugsLibraries(group, url) {
  const dir = join(SITE, "src", "content", "docs", "docs", "compared-with-original", "libraries", group);
  let names;
  try {
    names = readdirSync(dir, { withFileTypes: true });
  } catch (e) {
    if (e.code === "ENOENT") throw new Error(`fixed-bugs-libraries: no section "${group}"`);
    throw e;
  }
  // a page is NAME.md or, with its images, NAME/index.md
  const pages = names
    .map((d) => {
      if (d.isDirectory()) {
        const index = readdirSync(join(dir, d.name)).find((n) => /^index\.mdx?$/.test(n));
        return index && { name: d.name, file: join(dir, d.name, index) };
      }
      return /\.mdx?$/.test(d.name) && !/^index\./.test(d.name) && { name: d.name.replace(/\.mdx?$/, ""), file: join(dir, d.name) };
    })
    .filter(Boolean)
    .map(({ name, file }) => ({ key: `libraries/${group}/${name}`, title: frontMatter(file).title }))
    .sort((a, b) => byText(a.title, b.title));
  const body = pages
    .map(({ key, title }) => {
      const rows = fixoverview().rows.filter((r) => r.page === key);
      const ref = refpage(title, key);
      return `<tr><td><a href="${esc(url(`${FIXBASE}/${key}/`))}">${esc(title)}</a></td><td>${rows.length}</td><td>${levelBadges(rows)}</td><td>${
        ref ? `<a href="${esc(url(ref.path))}">${esc(ref.title)}</a>` : ""
      }</td></tr>`;
    })
    .join("");
  return `<div class="fixed-bugs-libraries"><table><thead><tr><th>Library</th><th>Fixes</th><th>Levels</th><th>Command reference</th></tr></thead><tbody>${body}</tbody></table></div>`;
}
