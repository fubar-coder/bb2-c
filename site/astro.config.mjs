// The website with Astro Starlight (site-starlight; Hugo, hugo.yaml, stays
// the published site until the cut-over). Built with Node.js in the pinned
// image docker.io/library/node:26.13.1-bookworm-slim via podman:
//
//   podman run --rm -v $PWD/site:/site -w /site \
//       docker.io/library/node:26.13.1-bookworm-slim sh -c 'npm ci && npm run build'
//
// The output is dist/; the build must finish without errors or warnings
// (ctest site_starlight). Astro's telemetry is off in the package scripts.
// The sidebar is built from the converted pages (src/lib/sidebar.mjs), so
// the converter (scripts/convert-hugo.mjs) runs before Astro reads this.
import { fileURLToPath } from "node:url";
import { defineConfig } from "astro/config";
import starlight from "@astrojs/starlight";
import mermaid from "astro-mermaid";
import { unified } from "@astrojs/markdown-remark";
import { remarkBb2 } from "./src/plugins/remark-bb2.mjs";
import { sidebar } from "./src/lib/sidebar.mjs";

export default defineConfig({
  site: "https://fubar-coder.github.io",
  base: "/bb2-c",
  trailingSlash: "always",
  // Hugo writes its output into public/, so Astro's static files come from
  // Hugo's static/ until the cut-over
  publicDir: "static",
  // remark/rehype, so that the site's remark plugins and astro-mermaid run;
  // Astro's default processor (Sätteri) has a plugin API of its own
  markdown: {
    processor: unified({ remarkPlugins: [remarkBb2] }),
  },
  vite: {
    build: {
      // Vite warns for a chunk above 500 kB. Above it are only chunks of
      // mermaid, which astro-mermaid imports on a page with a diagram: its
      // ELK layout (1501 kB minified, mermaid 12.3.0), its core (661 kB) and
      // its parser (644 kB). The limit sits just above the largest, so that
      // a larger chunk still warns (design D12)
      chunkSizeWarningLimit: 1510,
    },
  },
  integrations: [
    // ```mermaid blocks, rendered in the browser in the light or dark theme
    // of the page
    mermaid({ theme: "default", autoTheme: true, enableLog: false }),
    starlight({
      title: "BB2 C port",
      social: [{ icon: "github", label: "GitHub", href: "https://github.com/fubar-coder/bb2-c" }],
      customCss: ["./assets/css/fix-overview.css", "./assets/css/fixed-bugs.css"],
      components: { Head: "./src/components/Head.astro" },
      sidebar: sidebar(fileURLToPath(new URL("./src/content/docs", import.meta.url))),
      // blitzbasic: plain text until the Blitz Basic grammar (task 8.1);
      // basic: the fragments of test programs on the test pages, plain text
      // as in Hugo
      expressiveCode: {
        shiki: { langAlias: { blitzbasic: "plaintext", basic: "plaintext" } },
      },
    }),
  ],
});
