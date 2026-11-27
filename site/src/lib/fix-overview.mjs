// The overview of the fixed original bugs on the page "Important Fixes &
// Breaking Changes" (site-starlight, task 6.1): the rows of the component
// src/components/FixOverview.astro (the port of Hugo's shortcode
// layouts/_shortcodes/fix-overview.html).
//
// The data file is data/fixoverview.yaml, or the file the environment
// variable BB2_FIXOVERVIEW_DATA names relative to the site directory (the
// port of Hugo's site parameter fixOverviewData). Only the overview reads
// it; the pages of fixed library bugs and the command reference keep
// data/fixoverview.yaml (src/lib/fixed-bugs.mjs), as in Hugo.
import { readFileSync } from "node:fs";
import { join } from "node:path";
import YAML from "yaml";
import { SITE, fixoverview, libraries } from "./data.mjs";
import { rowHref, splitList } from "./fixed-bugs.mjs";

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
