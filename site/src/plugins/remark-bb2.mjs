// The site's remark plugin: the port of the Hugo features that work on the
// Markdown itself (site-starlight, design D4, D6).
//
// * Heading ids written as "## Heading {#id}" (Hugo's syntax; Astro's
//   Markdown has none): the id is set on the heading and the text taken
//   off. In an MDX page the converter writes it escaped ("\{#id\}"), which
//   reaches the plugin as the same text. Astro's heading-id step keeps an
//   id that is set.
//
// The site's directives (::fixed-bugs-library and the others) follow with
// task 5.1.
function walk(node, fn) {
  fn(node);
  for (const child of node.children ?? []) walk(child, fn);
}

export function remarkBb2() {
  return (tree) => {
    walk(tree, (node) => {
      if (node.type !== "heading") return;
      const last = node.children.at(-1);
      const m = last?.type === "text" && /\s*\{#([\w-]+)\}\s*$/.exec(last.value);
      if (!m) return;
      last.value = last.value.slice(0, m.index);
      node.data = { ...node.data, hProperties: { ...node.data?.hProperties, id: m[1] } };
    });
  };
}
