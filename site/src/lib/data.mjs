// The data of site/data, read where it is (nothing copied), for the pages
// and the checks of the Starlight site (site-starlight, design D8):
//
// * data/libraries/GROUP/NAME.json: the commands of a library (written by
//   the cmdref tool), GROUP acidlibs, userlibs or deflibs;
// * data/libintros/GROUP/NAME.yaml: the introduction of a library's
//   reference page; data/cmddesc/GROUP/NAME.yaml: the descriptions of its
//   commands;
// * data/fixoverview.yaml: the overview of fixed original bugs.
//
// The text fields of libintros and cmddesc are Markdown. Until the cut-over
// (task 9.1) they link with Hugo's relref shortcode; the readers here
// return them with every relref replaced by its site path ("/docs/a/b/#c",
// without the base, which the site's remark plugin adds when the text is
// rendered). At the cut-over the data files are converted to site paths
// and the resolver goes.
//
// The site directory: in the build, Vite bundles this module into the
// page chunks, which do not keep its path, so the site's integration
// (src/integrations/site-checks.mjs) defines __BB2_SITE__ as Astro's root;
// read directly by Node.js (the integration itself), the path comes from
// this file.
import { readFileSync, readdirSync } from "node:fs";
import { basename, join } from "node:path";
import { fileURLToPath } from "node:url";
import YAML from "yaml";
import { outsideCode } from "./markdown.mjs";
import { refPath } from "../../scripts/convert-hugo.mjs";

/* global __BB2_SITE__ */
export const SITE = typeof __BB2_SITE__ === "string" ? __BB2_SITE__ : fileURLToPath(new URL("../..", import.meta.url));
export const DATA = join(SITE, "data");
export const GROUPS = ["acidlibs", "userlibs", "deflibs"];

const cache = new Map();
function cached(key, fn) {
  if (!cache.has(key)) cache.set(key, fn());
  return cache.get(key);
}

/** The files NAME.ext of a directory, parsed: {NAME: value}, sorted by name */
function readDir(dir, ext, parse) {
  const out = {};
  let names;
  try {
    names = readdirSync(dir).sort();
  } catch (e) {
    if (e.code === "ENOENT") return out;
    throw e;
  }
  for (const f of names) if (f.endsWith(ext)) out[basename(f, ext)] = parse(readFileSync(join(dir, f), "utf8"));
  return out;
}

/** Hugo's relrefs of a Markdown text (outside code blocks) as site paths */
export function resolveRelrefs(md) {
  return outsideCode(md, (t) => t.replace(/\{\{<\s*relref\s+"([^"]+)"\s*>\}\}/g, (_, r) => refPath(r, "docs/data.md")));
}

/** value with resolveRelrefs applied to every string in it */
const resolveAll = (value) =>
  typeof value === "string"
    ? resolveRelrefs(value)
    : Array.isArray(value)
      ? value.map(resolveAll)
      : value && typeof value === "object"
        ? Object.fromEntries(Object.entries(value).map(([k, v]) => [k, resolveAll(v)]))
        : value;

/** data/fixoverview.yaml */
export const fixoverview = () => cached("fixoverview", () => YAML.parse(readFileSync(join(DATA, "fixoverview.yaml"), "utf8")));

/** data/libraries/GROUP/NAME.json: {GROUP: {NAME: library}} */
export const libraries = () =>
  cached("libraries", () => Object.fromEntries(GROUPS.map((g) => [g, readDir(join(DATA, "libraries", g), ".json", JSON.parse)])));

/** data/libintros/GROUP/NAME.yaml, relrefs resolved: {GROUP: {NAME: intro}} */
export const libintros = () =>
  cached("libintros", () =>
    Object.fromEntries(GROUPS.map((g) => [g, readDir(join(DATA, "libintros", g), ".yaml", (s) => resolveAll(YAML.parse(s) ?? {}))])),
  );

/** data/cmddesc/GROUP/NAME.yaml, relrefs resolved: {GROUP: {NAME: {command: desc}}} */
export const cmddesc = () =>
  cached("cmddesc", () =>
    Object.fromEntries(GROUPS.map((g) => [g, readDir(join(DATA, "cmddesc", g), ".yaml", (s) => resolveAll(YAML.parse(s) ?? {}))])),
  );

/** The levels of fixoverview.yaml by level ("1".."5") */
export const levels = () => cached("levels", () => Object.fromEntries(fixoverview().levels.map((l) => [String(l.level), l])));
/** The rows of fixoverview.yaml by issue */
export const rowsByIssue = () => cached("rowsByIssue", () => Object.fromEntries(fixoverview().rows.map((r) => [String(r.issue), r])));

/** Hugo's anchorize (goldmark, github style), used for every generated id */
export function anchorize(s) {
  return s
    .toLowerCase()
    .replace(/\s/g, "-")
    .replace(/[^\p{L}\p{N}_-]/gu, "");
}

/**
 * The heading ids of a command library's entries, in page order (sorted by
 * name), a repeated id with -1, -2, ...: [{entry, id}]
 */
export function entryIds(entries) {
  const seen = {};
  return [...entries]
    .sort((a, b) => (a.name < b.name ? -1 : a.name > b.name ? 1 : 0))
    .map((e) => {
      let id = anchorize(e.name);
      const n = seen[id] ?? 0;
      seen[id] = n + 1;
      if (n) id = `${id}-${n}`;
      return { entry: e, id };
    });
}

/** The directory of the reference pages of a group */
export const REFDIR = { acidlibs: "/docs/libraries/commands/", userlibs: "/docs/libraries/userlibs/", deflibs: "/docs/libraries/amigaos/" };

/**
 * The Markdown texts of libintros and cmddesc, relrefs resolved, each with
 * the data file, the place in it and the reference page that shows it (to
 * which a relative link is relative): [{file, where, page, text}]
 */
export function dataTexts() {
  const out = [];
  const collect = (file, page, where, value) => {
    if (typeof value === "string") out.push({ file, where, page, text: value });
    else if (value && typeof value === "object")
      for (const [k, v] of Object.entries(value)) collect(file, page, where ? `${where}.${k}` : k, v);
  };
  for (const [kind, data] of [
    ["libintros", libintros()],
    ["cmddesc", cmddesc()],
  ])
    for (const g of GROUPS)
      for (const [name, value] of Object.entries(data[g])) collect(`data/${kind}/${g}/${name}.yaml`, `${REFDIR[g]}${name}/`, "", value);
  return out;
}

/** The link targets of the Markdown links of a text, outside code blocks */
export function markdownLinks(md) {
  const links = [];
  outsideCode(md, (t) => {
    for (const m of t.replace(/`[^`\n]*`/g, "").matchAll(/\]\(\s*<?([^)\s>]+)/g)) links.push(m[1]);
    return t;
  });
  return links;
}

/** The links of the data texts: [{file, where, page, link}] */
export const dataLinks = () => dataTexts().flatMap(({ text, ...at }) => markdownLinks(text).map((link) => ({ ...at, link })));
