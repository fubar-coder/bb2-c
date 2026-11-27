// Converts the Hugo pages of site/content (Hextra theme) into the pages of
// the Starlight site in site/src/content/docs/ (ignored by git until the
// cut-over; site-starlight, design D2, D6). Run by `npm run build` before
// Astro, so that the Starlight build follows the Hugo pages:
//
//   node scripts/convert-hugo.mjs [--report FILE]
//
// What it does to a page:
//
// * `_index.md` becomes `index.md` (Starlight's loader skips files whose
//   name begins with "_"); the home page (content/_index.md) becomes
//   src/content/docs/index.mdx with the splash template and its hero.
// * relref shortcodes become site paths without the base ("/docs/a/b/#c";
//   the site's remark plugin adds the base, design D7), resolved as Hugo
//   does: an absolute ref from the content root, a relative one from the
//   page's directory.
// * callout -> an aside (`:::note`, `:::caution`, `:::danger`, `:::tip`),
//   details -> `<details><summary>`.
// * Hextra's cards, steps and filetree -> Starlight's CardGrid/LinkCard,
//   Steps and FileTree; such a page becomes `.mdx`, and its text outside
//   code is escaped for MDX (`{`, `}`, `<`; an autolink becomes a link).
// * The site's shortcodes (fixed-bugs-library, fixed-bugs-libraries,
//   library-list, fix-overview) -> leaf directives with the same
//   attributes (`::fixed-bugs-library{part="elsewhere"}`), handled by the
//   site's remark plugin.
// * Front matter: title, description; weight -> sidebar.order; prev/next
//   -> Starlight's {link, label} with the target page's title; aliases,
//   anchorMoves and width kept (src/content.config.mjs extends the schema).
// * Code blocks are left as they are; the shortcodes never occur in one.
//
// The other files of a page bundle (images) are copied next to the page.
// A shortcode the converter does not know, left outside a code block,
// stops it with the file and the line. --report writes the list of every
// rule applied, per page (the counts of task 2.2).
import { readFileSync, writeFileSync, mkdirSync, rmSync, readdirSync, copyFileSync, statSync } from "node:fs";
import { join, dirname, posix, relative } from "node:path";
import { fileURLToPath } from "node:url";
import YAML from "yaml";
import { segments, outsideCode, lineOf } from "../src/lib/markdown.mjs";

const SITE = join(dirname(fileURLToPath(import.meta.url)), "..");
const CONTENT = join(SITE, "content");
const OUT = join(SITE, "src", "content", "docs");

/** Every file below dir, as paths relative to dir with "/" */
function walk(dir, base = dir) {
  return readdirSync(dir).flatMap((name) => {
    const p = join(dir, name);
    return statSync(p).isDirectory() ? walk(p, base) : [relative(base, p).split("\\").join("/")];
  });
}

/** The page key of a content file: "docs/a/b" for docs/a/b.md, docs/a/b/_index.md, docs/a/b/index.md */
const pageKey = (file) => file.replace(/\.md$/, "").replace(/\/?_?index$/, "");

/**
 * The site path of a ref ("/docs/a/b#c" or relative to the page): "/docs/a/b/#c".
 * Also used for the relrefs of the data files until the cut-over
 * (src/lib/data.mjs).
 */
export function refPath(target, file) {
  let [path, anchor] = target.split("#");
  if (path === "") path = pageKey(file);
  else if (!path.startsWith("/")) path = posix.join(dirname(file), path);
  path = path.replace(/^\/+/, "").replace(/\.md$/, "").replace(/\/?_?index$/, "").replace(/\/$/, "");
  return `/${path}${path ? "/" : ""}${anchor ? `#${anchor}` : ""}`;
}

/** The site path of a Hextra card or button link, relative to the page's URL */
function cardPath(link, file) {
  if (/^[a-z]+:/.test(link) || link.startsWith("/")) return link;
  const key = pageKey(file);
  const url = new URL(link, `https://site/${key}${key ? "/" : ""}`);
  return url.pathname.replace(/\/?$/, "/") + url.hash;
}

const attrs = (s) => Object.fromEntries([...s.matchAll(/(\w+)="([^"]*)"/g)].map((m) => [m[1], m[2]]));

/** Indents every line that is not empty */
const indent = (text, n) => text.replace(/^(?=[^\n])/gm, " ".repeat(n));

/** A JSX attribute: "value" or {"value"} when it holds a quote */
const jsxAttr = (name, value) => (value.includes('"') ? `${name}={${JSON.stringify(value)}}` : `${name}="${value}"`);

/**
 * Escapes the text of an MDX page outside code (fences and inline code)
 * and outside the shortcodes: `{`, `}`, `<` become `\{`, `\}`, `\<`; an
 * autolink <https://x> becomes [https://x](https://x). count(kind) is
 * called for each change.
 */
function escapeMdx(src, count) {
  return outsideCode(src, (t) =>
    t.replace(/(`+)[\s\S]*?\1|\{\{[<%][\s\S]*?[>%]\}\}|<(https?:\/\/[^>\s]+)>|[{}<]/g, (m, _tick, auto) => {
      if (auto) {
        count("autolink");
        return `[${auto}](${auto})`;
      }
      if (m.length > 1) return m;
      count(m === "<" ? "less-than" : "brace");
      return `\\${m}`;
    }),
  );
}

/**
 * Converts one page. file: path below site/content ("docs/a/b.md"); titles:
 * page key -> title (for prev/next labels). Returns {out, text, applied}
 * with out the path below src/content/docs and applied the list of rules.
 */
export function convertPage(file, raw, titles) {
  const applied = [];
  const count = (rule) => applied.push(rule);
  const m = raw.match(/^---\n([\s\S]*?)\n---\n/);
  const fm = m ? YAML.parse(m[1]) : {};
  let body = m ? raw.slice(m[0].length) : raw;
  const home = file === "_index.md";
  const components = new Set();
  const mdx = home || /\{\{[<%]\s*(cards|steps|filetree\/container)\b/.test(body);

  const data = { title: fm.title };
  if (fm.description) data.description = fm.description;
  if (home) {
    // the hero (headline, subtitle, button) into the splash template's front
    // matter; the layout's <div> wrappers go, the section heading becomes a
    // heading; the feature grids are card grids (below)
    const sc = (name) => new RegExp(`\\{\\{<\\s*hextra/${name}\\s*>\\}\\}([\\s\\S]*?)\\{\\{<\\s*/hextra/${name}\\s*>\\}\\}`).exec(body)?.[1].trim();
    const button = attrs(/\{\{<\s*hextra\/hero-button\s+([^>]*?)>\}\}/.exec(body)?.[1] ?? "");
    const tagline = sc("hero-subtitle")?.replace(/&nbsp;/g, " ").replace(/<br[^>]*>/g, "").replace(/\s+/g, " ");
    data.template = "splash";
    data.hero = { title: sc("hero-headline"), tagline };
    if (button.text) data.hero.actions = [{ text: button.text, link: cardPath(button.link, file), variant: "primary" }];
    count("home:hero");
    body = body
      .replace(/\{\{<\s*hextra\/hero-(headline|subtitle)\s*>\}\}[\s\S]*?\{\{<\s*\/hextra\/hero-\1\s*>\}\}/g, "")
      .replace(/\{\{<\s*hextra\/hero-button[^>]*>\}\}/g, "")
      .replace(/\{\{<\s*hextra\/hero-section\s+heading="h(\d)"\s*>\}\}([\s\S]*?)\{\{<\s*\/hextra\/hero-section\s*>\}\}/g, (_, n, t) => `${"#".repeat(+n)} ${t.trim()}`)
      .replace(/^<div[^>]*>\s*(.*?)\s*<\/div>\n/gms, (_, inner) => (inner ? `${inner}\n` : ""))
      .replace(/\n{3,}/g, "\n\n");
  }

  if (mdx) body = escapeMdx(body, (k) => count(`mdx-escape:${k}`));

  // the paired Hextra shortcodes enclose code blocks: on the whole text
  body = body.replace(/\{\{%\s*details\s+([^%]*?)%\}\}([\s\S]*?)\{\{%\s*\/details\s*%\}\}|\{\{<\s*details\s+([^>]*?)>\}\}([\s\S]*?)\{\{<\s*\/details\s*>\}\}/g, (_, a1, i1, a2, i2) => {
    const { title = "Details", closed } = attrs(a1 ?? a2);
    count("details");
    return `<details${closed === "true" ? "" : " open"}>\n<summary>${title}</summary>\n\n${(i1 ?? i2).trim()}\n\n</details>`;
  });
  body = body.replace(/\{\{<\s*callout\s+([^>]*?)>\}\}([\s\S]*?)\{\{<\s*\/callout\s*>\}\}/g, (_, a, inner) => {
    const type = attrs(a).type ?? "info";
    const aside = { info: "note", warning: "caution", error: "danger", important: "tip" }[type];
    if (!aside) throw new Error(`${file}: callout type "${type}" unknown`);
    count(`callout:${type}`);
    return `:::${aside}\n${inner.trim()}\n:::`;
  });
  // filetree: the nested folders and files as a nested list
  body = body.replace(/\{\{<\s*filetree\/container\s*>\}\}([\s\S]*?)\{\{<\s*\/filetree\/container\s*>\}\}/g, (_, inner) => {
    const lines = [];
    let depth = 0;
    const tokens = inner.matchAll(/\{\{<\s*(\/?)filetree\/(folder|file)\s*([^>]*?)>\}\}/g);
    let open = null; // a folder whose children are not seen yet
    for (const [, close, kind, a] of tokens) {
      if (close) {
        if (open) {
          // a folder without children: Starlight shows it closed with "…"
          lines.push(open.line.replace(/^(\s*- \S+)/, "$1/"));
          open = null;
        }
        depth--;
        continue;
      }
      if (open) {
        // Starlight opens every folder that has children
        if (open.state === "closed") count("layout:closed-folder-shown-open");
        lines.push(open.line);
        open = null;
      }
      const { name, state } = attrs(a);
      const [first, ...rest] = name.split(" ");
      const line = `${"  ".repeat(depth)}- ${first}${rest.length ? ` ${rest.join(" ")}` : ""}`;
      if (kind === "folder") {
        open = { line, state };
        if (state === "closed") count("filetree:closed-folder");
        depth++;
      } else lines.push(line);
    }
    if (depth !== 0 || open) throw new Error(`${file}: filetree not balanced`);
    count("filetree");
    components.add("FileTree");
    return `<FileTree>\n\n${lines.join("\n")}\n\n</FileTree>`;
  });
  // cards
  body = body.replace(/\{\{<\s*cards\s*>\}\}([\s\S]*?)\{\{<\s*\/cards\s*>\}\}/g, (_, inner) => {
    const cards = [...inner.matchAll(/\{\{<\s*card\s+([^>]*?)>\}\}/g)].map(([, a]) => {
      const { link, title, subtitle } = attrs(a);
      count("card");
      return `  <LinkCard ${jsxAttr("title", title)} ${jsxAttr("href", cardPath(link, file))}${subtitle ? ` ${jsxAttr("description", subtitle)}` : ""} />`;
    });
    components.add("CardGrid").add("LinkCard");
    return `<CardGrid>\n${cards.join("\n")}\n</CardGrid>`;
  });
  // steps: every heading of the level of the first one begins a step, an item of an ordered list
  body = body.replace(/\{\{%\s*steps\s*%\}\}([\s\S]*?)\{\{%\s*\/steps\s*%\}\}/g, (_, inner) => {
    // the level of the steps' headings: that of the first heading
    const level = /^(#{1,6}) /m.exec(inner)?.[1] ?? "###";
    const start = new RegExp(`^(?=${level} )`, "m");
    const parts = [];
    let pre = "";
    for (const s of segments(inner.trim() + "\n")) {
      if (s.code) {
        if (parts.length) parts[parts.length - 1] += s.text;
        else pre += s.text;
        continue;
      }
      const pieces = s.text.split(start);
      if (parts.length) parts[parts.length - 1] += pieces.shift();
      else if (!pieces[0].startsWith(`${level} `)) pre += pieces.shift();
      parts.push(...pieces);
    }
    if (pre.trim()) throw new Error(`${file}: text in steps before the first heading`);
    count("steps");
    components.add("Steps");
    const items = parts.map((p) => `1. ${indent(p.trimEnd(), 3).slice(3)}`);
    return `<Steps>\n\n${items.join("\n\n")}\n\n</Steps>`;
  });

  if (home) {
    body = body.replace(/\{\{<\s*hextra\/feature-grid[^>]*>\}\}([\s\S]*?)\{\{<\s*\/hextra\/feature-grid\s*>\}\}/g, (_, inner) => {
        const cards = [...inner.matchAll(/\{\{<\s*hextra\/feature-card\s+([^>]*?)>\}\}/g)].map(([, a]) => {
          const { title, subtitle, link } = attrs(a);
          count("home:feature-card");
          return `  <LinkCard ${jsxAttr("title", title)} ${jsxAttr("href", cardPath(link, file))} ${jsxAttr("description", subtitle)} />`;
        });
        components.add("CardGrid").add("LinkCard");
        return `<CardGrid>\n${cards.join("\n")}\n</CardGrid>`;
      });
  }

  body = outsideCode(body, (t) =>
    t
      .replace(/\{\{<\s*relref\s+"([^"]+)"\s*>\}\}/g, (_, r) => {
        count("relref");
        return refPath(r, file);
      })
      // the site's shortcodes: leaf directives
      .replace(/\{\{[<%]\s*(fixed-bugs-library|fixed-bugs-libraries|fix-overview|library-list)\s*([^>%]*?)\s*[>%]\}\}/g, (_, name, a) => {
        const at = Object.entries(attrs(a)).map(([k, v]) => `${k}="${v}"`).join(" ");
        count(`directive:${name}`);
        return `::${name}${at ? `{${at}}` : ""}`;
      }),
  );

  // nothing of Hugo may be left outside the code blocks
  let off = 0;
  for (const s of segments(body)) {
    const left = s.code ? -1 : s.text.search(/\{\{[<%]/);
    if (left >= 0) {
      const at = lineOf(body, off + left);
      throw new Error(`${file}: a shortcode the converter does not know, line ${at} of the output: ${s.text.slice(left, left + 60)}`);
    }
    off += s.text.length;
  }

  if (fm.weight !== undefined) data.sidebar = { order: fm.weight };
  for (const k of ["prev", "next"]) {
    if (!fm[k]) continue;
    const key = refPath(fm[k], file).replace(/^\/|\/$/g, "");
    if (!(key in titles)) throw new Error(`${file}: ${k} names ${fm[k]}, which is no page`);
    data[k] = { link: `/${key}/`, label: titles[key] };
    count(k);
  }
  for (const k of ["aliases", "anchorMoves", "width"]) {
    if (fm[k] === undefined) continue;
    data[k] = fm[k];
    count(`frontmatter:${k}`);
  }
  const known = new Set(["title", "description", "weight", "prev", "next", "aliases", "anchorMoves", "width", "layout"]);
  for (const k of Object.keys(fm)) if (!known.has(k)) throw new Error(`${file}: front matter key "${k}" unknown`);

  let out = home ? "index.mdx" : file.replace(/(^|\/)_index\.md$/, "$1index.md");
  if (mdx) {
    out = out.replace(/\.md$/, ".mdx");
    if (components.size) body = `import { ${[...components].sort().join(", ")} } from "@astrojs/starlight/components";\n\n${body.replace(/^\n+/, "")}`;
    count("mdx");
  }
  return { out, text: `---\n${YAML.stringify(data).trim()}\n---\n\n${body.replace(/^\n+/, "")}`, applied };
}

function main() {
  const reportAt = process.argv.indexOf("--report");
  const files = walk(CONTENT);
  const pages = files.filter((f) => f.endsWith(".md"));
  const titles = {};
  for (const f of pages) {
    const fm = readFileSync(join(CONTENT, f), "utf8").match(/^---\n([\s\S]*?)\n---\n/);
    titles[pageKey(f)] = YAML.parse(fm?.[1] ?? "")?.title;
  }
  rmSync(join(OUT, "docs"), { recursive: true, force: true });
  rmSync(join(OUT, "index.mdx"), { force: true });
  const report = [];
  for (const f of files) {
    if (f.endsWith(".gotmpl")) continue; // the content adapter of the command reference (task 5.2)
    if (!f.endsWith(".md")) {
      const to = join(OUT, f);
      mkdirSync(dirname(to), { recursive: true });
      copyFileSync(join(CONTENT, f), to);
      continue;
    }
    const page = convertPage(f, readFileSync(join(CONTENT, f), "utf8"), titles);
    const to = join(OUT, page.out);
    mkdirSync(dirname(to), { recursive: true });
    writeFileSync(to, page.text);
    report.push({ file: `content/${f}`, out: `src/content/docs/${page.out}`, applied: page.applied });
  }
  if (reportAt > 0) writeFileSync(process.argv[reportAt + 1], JSON.stringify(report, null, 1));
  console.log(`convert-hugo: ${report.length} pages converted`);
}

if (process.argv[1] === fileURLToPath(import.meta.url)) main();
