// The link check of the Starlight site (site-starlight, design D7, D9):
// every internal link of the built pages and of the library data, with its
// anchor. Run by the site's integration (src/integrations/site-checks.mjs)
// over the output HTML, which is what a reader gets, so it sees every link
// a page has: the Markdown links, the cards, the pager, the hero's buttons,
// Starlight's sidebar and table of contents.
//
// A link is internal when it has no scheme and no host. It must lead below
// the site's base (a site path whose base was not added does not), to a
// page of the output (a directory with index.html) or a file of it, and its
// anchor must be an id of the target page.
//
// The links of the data (libintros, cmddesc: src/lib/data.mjs) are checked
// as written, with the base added to a site path, against the same pages
// (relative to the reference page that shows them); they are named by
// their data file, which is where a writer fixes them.
import { existsSync, readdirSync, readFileSync, statSync } from "node:fs";
import { join } from "node:path";
import { dataLinks } from "./data.mjs";

/** The HTML entities of attribute values the output uses */
const decode = (s) =>
  s.replace(/&(#x[0-9a-f]+|#\d+|amp|lt|gt|quot|apos);/gi, (m, e) =>
    e[0] === "#"
      ? String.fromCodePoint(e[1] === "x" || e[1] === "X" ? parseInt(e.slice(2), 16) : +e.slice(1))
      : { amp: "&", lt: "<", gt: ">", quot: '"', apos: "'" }[e.toLowerCase()],
  );

/** Every file below dir, as paths relative to dir with "/" */
function walk(dir) {
  return readdirSync(dir).flatMap((name) => {
    const p = join(dir, name);
    return statSync(p).isDirectory() ? walk(p).map((f) => `${name}/${f}`) : [name];
  });
}

// The pages that come with later tasks of site-starlight: a link to one of
// them is counted, not failed, while the page is missing ({task,
// match(sitePath, id)}). A task removes its entry when it builds the pages,
// so that from then on the links are checked like every other (design.md,
// "Findings", group 4). None is left: the command reference (task 5.2) was
// the last.
const AWAITED = [];

/**
 * Checks the links of the built site in dist (a directory) with the base
 * base ("/bb2-c/"). Returns {errors, links, awaited} with errors the
 * messages (each naming the page or data file and the link), links the
 * number of internal links checked and awaited {task: count}.
 */
export function checkLinks(dist, base) {
  const pages = new Map(); // URL path -> {ids, html file}
  for (const f of walk(dist).filter((f) => f.endsWith(".html"))) {
    const url = base + (f === "index.html" ? "" : f.endsWith("/index.html") ? f.slice(0, -"index.html".length) : f);
    const html = readFileSync(join(dist, f), "utf8");
    pages.set(url, { html, ids: new Set([...html.matchAll(/\sid="([^"]*)"/g)].map((m) => decode(m[1]))) });
  }

  const errors = new Set();
  const awaited = {};
  let links = 0;
  /** null if href (on the page at URL path from) is good, else what is wrong */
  const check = (href, from) => {
    const u = new URL(href, `http://site${from}`);
    if (u.origin !== "http://site") return null;
    links++;
    let path;
    let id;
    try {
      path = decodeURI(u.pathname);
      id = decodeURIComponent(u.hash.slice(1));
    } catch {
      return "is not a valid URL";
    }
    if (!path.startsWith(base)) return `is outside the site's base ${base}`;
    const site = path.slice(base.length - 1); // the site path, "/docs/a/"
    const page = pages.get(path);
    if (!page) {
      if (!path.endsWith("/") && existsSync(join(dist, path.slice(base.length)))) return id ? `has an anchor on a file that is no page` : null;
      const a = AWAITED.find((a) => a.match(site, id));
      if (!a) return "leads to no page";
      awaited[a.task] = (awaited[a.task] ?? 0) + 1;
      return null;
    }
    if (id && !page.ids.has(id)) return `names the id "${id}", which ${path} does not have`;
    return null;
  };

  for (const [url, { html }] of pages) {
    for (const [tag] of html.matchAll(/<a\s[^>]*>/g)) {
      const href = /\shref="([^"]*)"/.exec(tag)?.[1];
      if (href === undefined) continue;
      const bad = check(decode(href), url);
      if (bad) errors.add(`page ${url}: the link ${decode(href)} ${bad}`);
    }
  }
  for (const { file, where, page, link } of dataLinks()) {
    const href = link.startsWith("/") && !link.startsWith("//") ? base.replace(/\/$/, "") + link : link;
    const bad = check(href, base.replace(/\/$/, "") + page);
    if (bad) errors.add(`${file} (${where}): the link ${link} ${bad}`);
  }
  return { errors: [...errors], links, awaited };
}

