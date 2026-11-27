// The old URLs of the Starlight site (site-starlight, design D10), run by
// the site's integration (src/integrations/site-checks.mjs) in
// astro:build:done, after the link check:
//
// * writeRedirects: the redirect pages. Every URL of a page's front matter
//   `aliases` gets a page that sends the reader to the page, with the HTML
//   of the former Hugo site's redirect pages: JavaScript keeps the anchor of the old
//   link, and an anchor named in the page's `anchorMoves` (heading id ->
//   a page below /docs/compared-with-original/) goes to that page instead;
//   without JavaScript a meta refresh and a link lead to the page, without
//   the anchor; `noindex` keeps the page out of search engines. An
//   `anchorMoves` naming a page that is not built, an alias that is a page
//   of the site and an alias of two pages fail the build.
// * checkUrls: every URL of the lists in tests/urls/ (tests/urls/hugo.txt:
//   the pages and redirect pages of the Hugo build, written once from its
//   output) must be in the output, as a page (a directory with index.html)
//   or a file. A page that is removed on purpose deletes its line in the
//   same commit (site/README.md).
import { existsSync, mkdirSync, readdirSync, readFileSync, statSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import YAML from "yaml";

/** Every file below dir, as paths relative to dir with "/" */
function walk(dir) {
  return readdirSync(dir).flatMap((name) => {
    const p = join(dir, name);
    return statSync(p).isDirectory() ? walk(p).map((f) => `${name}/${f}`) : [name];
  });
}

/** The file of a site path ("/docs/a/", "/404.html") in dist, or null */
const outputFile = (dist, path) => {
  const f = join(dist, path.endsWith("/") ? `${path}index.html` : path);
  return existsSync(f) && statSync(f).isFile() ? f : null;
};

/** The pages of src/content/docs (dir) with `aliases` or `anchorMoves`: [{path, aliases, anchorMoves}] */
function pagesWithMoves(dir) {
  const pages = [];
  for (const f of walk(dir).filter((f) => /\.mdx?$/.test(f))) {
    const fm = /^---\r?\n([\s\S]*?)\r?\n---\r?\n/.exec(readFileSync(join(dir, f), "utf8"));
    if (!fm) continue;
    const data = YAML.parse(fm[1]) ?? {};
    if (!data.aliases && !data.anchorMoves) continue;
    // the page's URL as Starlight's loader makes it: the file's path
    // without the extension and an index
    const slug = f.replace(/\.mdx?$/, "").replace(/(^|\/)index$/, "");
    pages.push({ path: `/${slug}${slug ? "/" : ""}`, aliases: data.aliases ?? [], anchorMoves: data.anchorMoves ?? {} });
  }
  return pages;
}

const escapeHtml = (s) => s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
/** A value as a JavaScript literal that is safe inside <script> */
const jsLiteral = (v) => JSON.stringify(v).replace(/</g, "\\u003c");

/** The redirect page to the absolute URL to with the moved anchors moves {id: absolute URL} */
function redirectHtml(to, moves) {
  const t = escapeHtml(to);
  return `<!DOCTYPE html>
<html lang="en">
<head>
<title>${t}</title>
<link rel="canonical" href="${t}">
<meta name="robots" content="noindex">
<meta charset="utf-8">
<script>(function () { var h = location.hash, m = ${jsLiteral(moves)}, k = decodeURIComponent(h.slice(1)), to = Object.prototype.hasOwnProperty.call(m, k) ? m[k] : ""; location.replace((to || ${jsLiteral(to)}) + h); })();</script>
<meta http-equiv="refresh" content="0; url=${t}">
</head>
<body><p>This page has moved to <a href="${t}">${t}</a>.</p></body>
</html>
`;
}

/**
 * Writes the redirect pages of the pages of contentDir (src/content/docs)
 * into dist, for the site origin ("https://fubar-coder.github.io") and the
 * base ("/bb2-c/"). Returns {errors, written}: errors the messages (each
 * naming the page), written the number of redirect pages.
 */
export function writeRedirects(contentDir, dist, origin, base) {
  const errors = [];
  const pages = pagesWithMoves(contentDir);
  const absolute = (path) => `${origin}${base.replace(/\/$/, "")}${path}`;
  const planned = new Map(); // alias -> the page's URL path
  for (const page of pages) {
    const name = base.replace(/\/$/, "") + page.path;
    if (!outputFile(dist, page.path)) {
      errors.push(`page ${name}: it has aliases or anchorMoves, but is not in the output`);
      continue;
    }
    const moves = {};
    for (const id of Object.keys(page.anchorMoves).sort()) {
      const target = `/docs/compared-with-original/${String(page.anchorMoves[id]).replace(/^\/+|\/+$/g, "")}/`;
      if (!outputFile(dist, target))
        errors.push(`page ${name}: anchorMoves names the page "${page.anchorMoves[id]}" (${id}), which does not exist`);
      else moves[id] = absolute(target);
    }
    for (const a of page.aliases) {
      const alias = `/${String(a).replace(/^\/+/, "")}`.replace(/^([^.]*[^/.])$/, "$1/");
      if (planned.has(alias)) errors.push(`page ${name}: the alias ${alias} is an alias of ${base.replace(/\/$/, "")}${planned.get(alias).page.path} too`);
      else if (outputFile(dist, alias)) errors.push(`page ${name}: the alias ${alias} is a page of the site`);
      else planned.set(alias, { page, moves });
    }
  }
  if (errors.length) return { errors, written: 0 };
  for (const [alias, { page, moves }] of planned) {
    const file = join(dist, alias.endsWith("/") ? `${alias}index.html` : alias);
    mkdirSync(dirname(file), { recursive: true });
    writeFileSync(file, redirectHtml(absolute(page.path), moves));
  }
  return { errors, written: planned.size };
}

// The pages that come with later tasks of site-starlight: an old URL of
// one of them is counted, not failed, while the page is missing. A task
// removes its entry when it builds the pages (design.md, "Findings",
// group 7). None is left: the command reference (task 5.2) was the last.
const AWAITED = [];

/**
 * Checks that every URL of the lists listDir/*.txt (site paths without the
 * base, one per line, "#" starts a comment) is in dist. Returns {errors,
 * urls, awaited}: errors the messages (each naming the list and the URL),
 * urls the number checked, awaited {task: count}.
 */
export function checkUrls(listDir, dist) {
  const errors = [];
  const awaited = {};
  let urls = 0;
  for (const list of readdirSync(listDir).filter((f) => f.endsWith(".txt")).sort()) {
    const lines = readFileSync(join(listDir, list), "utf8").split("\n");
    lines.forEach((line, i) => {
      const url = line.replace(/#.*/, "").trim();
      if (!url) return;
      urls++;
      if (outputFile(dist, url)) return;
      const a = AWAITED.find((a) => a.match(url));
      if (a) awaited[a.task] = (awaited[a.task] ?? 0) + 1;
      else errors.push(`tests/urls/${list}:${i + 1}: the old URL ${url} is not in the output`);
    });
  }
  return { errors, urls, awaited };
}
