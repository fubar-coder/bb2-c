// The site's remark plugin: the port of the Hugo features that work on the
// Markdown itself (site-starlight, design D4, D6, D7). Options: {base}, the
// site's base path ("/bb2-c").
//
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
// The site's directives (::fixed-bugs-library and the others) follow with
// task 5.1.
function walk(node, fn) {
  fn(node);
  for (const child of node.children ?? []) walk(child, fn);
}

/** The components whose href is a link */
const LINKING = new Set(["LinkCard", "LinkButton"]);

export function remarkBb2({ base = "" } = {}) {
  const prefix = base.replace(/\/$/, "");
  const withBase = (url) => (url.startsWith("/") && !url.startsWith("//") ? prefix + url : url);
  return (tree) => {
    walk(tree, (node) => {
      if (node.type === "link" || node.type === "definition") {
        node.url = withBase(node.url);
        return;
      }
      if ((node.type === "mdxJsxFlowElement" || node.type === "mdxJsxTextElement") && LINKING.has(node.name)) {
        for (const a of node.attributes ?? []) if (a.type === "mdxJsxAttribute" && a.name === "href" && typeof a.value === "string") a.value = withBase(a.value);
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
