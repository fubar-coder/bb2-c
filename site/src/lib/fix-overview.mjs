// The overview of the fixed original bugs on the page "Important Fixes &
// Breaking Changes" (site-starlight, tasks 6.1, 6.2, design D9): the rows
// of the component src/components/FixOverview.astro (the port of Hugo's
// shortcode layouts/_shortcodes/fix-overview.html) and the check of the
// data against the pages of fixed bugs (the port of
// layouts/_partials/fix-overview/check.html), which the site's integration
// (src/integrations/site-checks.mjs) runs after the build.
//
// The data file is data/fixoverview.yaml, or the file the environment
// variable BB2_FIXOVERVIEW_DATA names relative to the site directory (the
// broken data files of the check's tests, tests/fixoverview/data/CASE.yaml;
// the port of Hugo's site parameter fixOverviewData). Only the overview
// reads it; the pages of fixed library bugs and the command reference keep
// data/fixoverview.yaml (src/lib/fixed-bugs.mjs), as in Hugo.
//
// The check fails for every problem Hugo's did, with the same messages
// (the data file named by its path instead of Hugo's data key):
//
// * a levels list that is not the five levels 1 to 5 in this order, each
//   with a name and a text;
// * a row without a required field, with a severity that is not one of the
//   levels, a topic that is not in the file's topics list, or a highlight
//   that is not a boolean;
// * an issue that is a row twice, or a row and a notRows entry, and a
//   notRows entry without a reason;
// * a row whose page is not built or whose anchor is no heading id of its
//   page (in the output HTML, which is what a reader gets);
// * an issue linked on a page of fixed bugs (Language, Expressions with its
//   sub-page, Compiled programs, Running blitz2, Editor, and Libraries with
//   the pages below it) that is neither a row nor on the notRows list: a
//   raw-text scan of the pages' Markdown (the converted pages of
//   src/content/docs/), code blocks included.
//
// The limits are Hugo's: an anchor is proven to exist, not that the entry
// is below it; an issue number in a code block counts as linked.
import { existsSync, readdirSync, readFileSync, statSync } from "node:fs";
import { join } from "node:path";
import YAML from "yaml";
import { SITE, fixoverview, libraries } from "./data.mjs";
import { FIXBASE, rowHref, splitList } from "./fixed-bugs.mjs";

/** The pages of fixed bugs, below FIXBASE; the rows name them (or a page below them) in "page" */
const FIXPAGES = ["language", "expressions", "compiled-programs", "running-blitz2", "editor", "libraries"];

/** The fields a row must have (not empty) */
const REQUIRED = ["issue", "severity", "topic", "library", "text", "short", "page", "anchor"];

let source;
/**
 * The overview's data file: {file, data}, file its path relative to the
 * site directory (for the messages).
 */
export function overviewData() {
  if (source) return source;
  const file = process.env.BB2_FIXOVERVIEW_DATA;
  if (!file) return (source = { file: "data/fixoverview.yaml", data: fixoverview() });
  let text;
  try {
    text = readFileSync(join(SITE, file), "utf8");
  } catch (e) {
    if (e.code === "ENOENT") throw new Error(`fix-overview: no data file "${file}"`);
    throw e;
  }
  return (source = { file, data: YAML.parse(text) ?? {} });
}

/**
 * The component of a row (blitz2 for the compiler, also for blitz2 as a
 * program; ted for the editor; else the library, a deflibs library of the
 * AmigaOS calls by the AmigaOS library it opens), its items joined by ", "
 */
export function component(row) {
  return String(row.library ?? "")
    .split(",")
    .map((s) => {
      const c = s.trim();
      if (c === "Compiler") return "blitz2";
      if (c === "TED") return "ted";
      if (row.page === "libraries/amigaos") return libraries().deflibs[c]?.opens || c;
      return c;
    })
    .join(", ");
}

/** Hugo's plainify: the text without its HTML tags */
export const plainify = (s) => String(s ?? "").replace(/<[^>]*>/g, "");

/**
 * The rows in the table's initial order (by severity, inside a severity by
 * issue number), each with the fields of the table: [{row, component,
 * commands, fixKey, href}], href without the base
 */
export function overviewRows(data) {
  return [...(data.rows ?? [])]
    .sort((a, b) => a.severity - b.severity || a.issue - b.issue)
    .map((row) => ({
      row,
      component: component(row),
      commands: row.command ? splitList(row.command) : [],
      fixKey: `${row.command ? `${row.command} ` : ""}${plainify(row.short)}`,
      href: rowHref(row),
    }));
}

let processor;
/**
 * Hugo's markdownify: the Markdown text as HTML, rendered by Astro's
 * Markdown processor (GFM and smartypants, as the pages are); a text of one
 * paragraph without its <p>
 */
export async function markdownify(md) {
  if (!processor) {
    const { createMarkdownProcessor } = await import("@astrojs/markdown-remark");
    processor = createMarkdownProcessor({ syntaxHighlight: false });
  }
  const html = (await (await processor).render(String(md ?? ""))).code.trim();
  const m = /^<p>([\s\S]*)<\/p>$/.exec(html);
  return m && !m[1].includes("<p>") ? m[1] : html;
}

/** The files of a page of the converted site (key "docs/a/b"): a.md(x) or a/index.md(x) */
function pageFile(docs, key) {
  for (const f of [`${key}.md`, `${key}.mdx`, `${key}/index.md`, `${key}/index.mdx`]) if (existsSync(join(docs, f))) return f;
  return null;
}

/** The page files below a directory of the converted site */
function below(docs, dir) {
  const out = [];
  const walk = (d) => {
    let names;
    try {
      names = readdirSync(join(docs, d)).sort();
    } catch (e) {
      if (e.code === "ENOENT") return;
      throw e;
    }
    for (const n of names) {
      const p = `${d}/${n}`;
      if (statSync(join(docs, p)).isDirectory()) walk(p);
      else if (/\.mdx?$/.test(n)) out.push(p);
    }
  };
  walk(dir);
  return out;
}

/** The site path of a page file ("docs/a/b/index.md" -> "/docs/a/b/") */
const sitePath = (f) => `/${f.replace(/\.mdx?$/, "").replace(/(^|\/)index$/, "")}/`.replace(/\/\/$/, "/");

/** The ids of the headings of a built page (URL path below the base), or null if it is not built */
function headingIds(dist, path) {
  const file = join(dist, path, "index.html");
  if (!existsSync(file)) return null;
  const html = readFileSync(file, "utf8");
  return new Set([...html.matchAll(/<h[1-6]\s[^>]*?\bid="([^"]*)"/g)].map((m) => m[1]));
}

/**
 * Checks the overview's data against the pages of fixed bugs: the site
 * directory site (the converted pages in src/content/docs/), the built site
 * in dist with the base base ("/bb2-c/"). Returns the messages.
 */
export function checkFixOverview(site, dist, base) {
  const { file: name, data: d } = overviewData();
  const errors = [];
  const fail = (m) => errors.push(`fixoverview (${name}): ${m}`);
  const docs = join(site, "src", "content", "docs");
  const fixDir = FIXBASE.slice(1);

  const fixPages = [];
  for (const p of FIXPAGES) {
    const f = pageFile(docs, `${fixDir}/${p}`);
    if (f) fixPages.push({ key: `${fixDir}/${p}`, file: f });
    else fail(`the page ${FIXBASE}/${p} does not exist`);
  }

  const topics = d.topics ?? [];
  const levels = [];
  for (const l of d.levels ?? []) {
    if (!(l.name && l.text)) fail(`level ${l.level} has no name or no text`);
    levels.push(l.level);
  }
  if (levels.join(",") !== "1,2,3,4,5") fail(`the levels are ${levels.join(",")}, not the five levels 1 to 5`);

  const seen = new Set();
  for (const row of d.rows ?? []) {
    const id = String(row.issue || "?");
    for (const k of REQUIRED) if (!row[k]) fail(`row #${id} has no ${k}`);
    if (!levels.includes(row.severity)) fail(`row #${id} has the severity ${row.severity}, not one of the levels`);
    if (!topics.includes(row.topic)) fail(`row #${id} has the topic ${JSON.stringify(row.topic)}, which is not in the topics list`);
    if ("highlight" in row && typeof row.highlight !== "boolean") fail(`row #${id} has the highlight ${JSON.stringify(String(row.highlight))}, not true or false`);
    if (seen.has(id)) fail(`issue #${id} has more than one row`);
    seen.add(id);
    if (row.page) {
      const ids = headingIds(dist, `${fixDir}/${row.page}`);
      if (!ids) fail(`row #${id} names the page ${JSON.stringify(row.page)}, which does not exist`);
      else if (!ids.has(row.anchor)) fail(`row #${id} names the anchor ${JSON.stringify(row.anchor)}, which is no heading on the page ${row.page}`);
    }
  }
  for (const n of d.notRows ?? []) {
    const id = String(n.issue);
    if (!n.reason) fail(`the notRows entry #${id} has no reason`);
    if (seen.has(id)) fail(`issue #${id} is a row and on the notRows list`);
    seen.add(id);
  }

  // the pages of fixed bugs and every page below them (the libraries' pages)
  const files = [...fixPages.map((p) => p.file)];
  for (const p of fixPages) for (const f of below(docs, p.key)) if (!files.includes(f)) files.push(f);
  for (const f of files) {
    const raw = readFileSync(join(docs, f), "utf8").replace(/^---\n[\s\S]*?\n---\n/, "");
    const found = new Set();
    for (const m of raw.matchAll(/github\.com\/fubar-coder\/bb2-c\/issues\/([0-9]+)/g)) {
      if (!seen.has(m[1]) && !found.has(m[1]))
        fail(`issue #${m[1]} is linked on ${base.replace(/\/$/, "")}${sitePath(f)} but has no row and is not on the notRows list`);
      found.add(m[1]);
    }
  }
  return errors;
}
