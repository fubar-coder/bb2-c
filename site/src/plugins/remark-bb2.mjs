// The site's remark plugin: the port of the Hugo features that work on the
// Markdown itself (site-starlight, design D4, D6, D7). Options: {base}, the
// site's base path ("/bb2-c").
//
// * The site's directives (the converter writes them for Hugo's
//   shortcodes; src/lib/fixed-bugs.mjs, src/lib/cmdref.mjs):
//   - ::fixed-bugs-library on a page of fixed library bugs: the head (the
//     fixes by level, the reference page); {part="elsewhere"}: the section
//     "Fixes described with other libraries", as Markdown, so that its
//     heading is in the table of contents, or nothing;
//   - ::fixed-bugs-libraries{group="commands"} (or "userlibs"): the table of
//     the pages of fixed bugs of the group;
//   - ::library-list{what="commands"} on an index page of the command
//     reference: the table of the section's libraries.
//   ::fix-overview is the overview page's (task 6.1).
// * On the pages of fixed library bugs (docs/compared-with-original/libraries/
//   and the pages below it), a link to an issue that has a
//   row in data/fixoverview.yaml is followed by the badge of the row's level
//   and its links to the command reference (Hugo's link hook render-link.html
//   of that section).
// * On the AmigaOS pages of the command reference, an argument list in a
//   table cell ("(rp,string,count)") may break after each comma (Hugo's
//   table hook render-table.html of that section).
// * Heading ids written as "## Heading {#id}" (Hugo's syntax; Astro's
//   Markdown has none): the id is set on the heading and the text taken
//   off. In an MDX page the converter writes it escaped ("\{#id\}"), which
//   reaches the plugin as the same text. Astro's heading-id step keeps an
//   id that is set.
// * Links: a page links with site paths ("/docs/a/b/#c"); the plugin adds
//   the base to them, in Markdown links and link definitions and in the
//   href of Starlight's LinkCard and LinkButton in an MDX page. The links of
//   the front matter get it from the route middleware
//   (src/lib/route-base.mjs); the link check of the build
//   (src/lib/links.mjs) finds a link without it.
//
// The page is the file's path below src/content/docs/ (the command
// reference's loader passes the path of its entry).
import { fromMarkdown } from "mdast-util-from-markdown";
import { sectionPages } from "../lib/cmdref.mjs";
import { ISSUE, esc, fixedBugsElsewhere, fixedBugsHead, fixedBugsLibraries, issueMeta, pageKey } from "../lib/fixed-bugs.mjs";

function walk(node, fn) {
  fn(node);
  for (const child of node.children ?? []) walk(child, fn);
}

/** The components whose href is a link */
const LINKING = new Set(["LinkCard", "LinkButton"]);

/** The site path of a page file ("docs/a/b"), or "" for a file outside src/content/docs/ */
function pageOf(file) {
  const path = (file.path ?? "").replace(/\\/g, "/");
  const i = path.lastIndexOf("/src/content/docs/");
  if (i < 0) return "";
  return path
    .slice(i + "/src/content/docs/".length)
    .replace(/\.mdx?$/, "")
    .replace(/(^|\/)index$/, "");
}

/** The table of the libraries of a reference section (the shortcode library-list) */
function libraryList(section, what, url) {
  const pages = sectionPages(section);
  const about = pages.some((p) => p.about);
  const rows = pages
    .map(
      (p) =>
        `<tr><td><a href="${esc(url(`/${p.id}/`))}">${esc(p.title)}</a></td><td>${p.number}</td><td>${p.count}</td>${about ? `<td>${esc(p.about)}</td>` : ""}</tr>`,
    )
    .join("");
  const head = `<tr><th>Library</th><th>Number</th><th>${esc(what.charAt(0).toUpperCase() + what.slice(1))}</th>${about ? "<th>About</th>" : ""}</tr>`;
  return `<table><thead>${head}</thead><tbody>${rows}</tbody></table>`;
}

/** The nodes that replace a directive of the site, or undefined for another directive */
function directive(node, page, url, fail) {
  const a = node.attributes ?? {};
  switch (node.name) {
    case "fixed-bugs-library": {
      if (!page.startsWith("docs/compared-with-original/libraries/")) fail(`::fixed-bugs-library on a page that is no page of fixed library bugs`);
      const key = pageKey(page);
      if ((a.part ?? "head") === "head") return [{ type: "html", value: fixedBugsHead(key, url) }];
      if (a.part !== "elsewhere") fail(`::fixed-bugs-library: unknown part "${a.part}"`);
      const md = fixedBugsElsewhere(key);
      return md ? fromMarkdown(md).children : [];
    }
    case "fixed-bugs-libraries":
      if (!["commands", "userlibs"].includes(a.group)) fail(`::fixed-bugs-libraries: unknown group "${a.group}"`);
      return [{ type: "html", value: fixedBugsLibraries(a.group, url) }];
    case "library-list": {
      const section = /^docs\/libraries\/(commands|userlibs|amigaos)$/.exec(page)?.[1];
      if (!section) fail(`::library-list on a page that is no index page of the command reference`);
      return [{ type: "html", value: libraryList(section, a.what ?? "commands", url) }];
    }
  }
  return undefined;
}

export function remarkBb2({ base = "" } = {}) {
  const prefix = base.replace(/\/$/, "");
  const withBase = (url) => (url.startsWith("/") && !url.startsWith("//") ? prefix + url : url);
  return (tree, file) => {
    const page = pageOf(file);
    const fail = (what) => {
      throw new Error(`${file.path}: ${what}`);
    };
    const fixPage = /^docs\/compared-with-original\/libraries(\/|$)/.test(page);
    const osPage = page.startsWith("docs/libraries/amigaos/");

    // the directives first, so that the links of the Markdown they give get
    // the base below
    walk(tree, (node) => {
      if (!node.children) return;
      for (let i = 0; i < node.children.length; i++) {
        const c = node.children[i];
        if (c.type !== "leafDirective") continue;
        const nodes = directive(c, page, withBase, fail);
        if (nodes) {
          node.children.splice(i, 1, ...nodes);
          i += nodes.length - 1;
        }
      }
    });

    walk(tree, (node) => {
      if (node.type === "link" || node.type === "definition") {
        node.url = withBase(node.url);
        return;
      }
      if ((node.type === "mdxJsxFlowElement" || node.type === "mdxJsxTextElement") && LINKING.has(node.name)) {
        for (const a of node.attributes ?? []) if (a.type === "mdxJsxAttribute" && a.name === "href" && typeof a.value === "string") a.value = withBase(a.value);
        return;
      }
      if (fixPage && node.children)
        for (let i = 0; i < node.children.length; i++) {
          const c = node.children[i];
          const issue = c.type === "link" && ISSUE.exec(c.url)?.[1];
          const html = issue && issueMeta(issue, withBase);
          if (html) node.children.splice(++i, 0, { type: "html", value: html });
        }
      if (osPage && node.type === "tableCell") {
        const [c, ...rest] = node.children;
        if (c?.type === "inlineCode" && c.value.startsWith("(") && !rest.length)
          node.children = [{ type: "html", value: `<code>${esc(c.value).replaceAll(",", ",<wbr>")}</code>` }];
        return;
      }
      if (node.type !== "heading") return;
      const last = node.children.at(-1);
      const m = last?.type === "text" && /\s*\{#([\w-]+)\}\s*$/.exec(last.value);
      if (!m) return;
      last.value = last.value.slice(0, m.index);
      node.data = { ...node.data, hProperties: { ...node.data?.hProperties, id: m[1] } };
    });
  };
}
