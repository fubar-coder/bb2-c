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
import { remarkTypographer } from "./src/plugins/remark-typographer.mjs";
import { sidebar } from "./src/lib/sidebar.mjs";
import { blitzbasicGrammar } from "./src/lib/grammar.mjs";
import { siteChecks } from "./src/integrations/site-checks.mjs";

const base = "/bb2-c";

export default defineConfig({
  site: "https://fubar-coder.github.io",
  base,
  trailingSlash: "always",
  // Hugo writes its output into public/, so Astro's static files come from
  // Hugo's static/ until the cut-over
  publicDir: "static",
  // remark/rehype, so that the site's remark plugins and astro-mermaid run;
  // Astro's default processor (Sätteri) has a plugin API of its own. The
  // typography is Hugo's (src/plugins/remark-typographer.mjs), not Astro's
  // smartypants
  markdown: {
    processor: unified({ smartypants: false, remarkPlugins: [[remarkBb2, { base }], remarkTypographer] }),
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
      // the base on the links of the front matter; the checks of the build
      plugins: [siteChecks()],
      social: [{ icon: "github", label: "GitHub", href: "https://github.com/fubar-coder/bb2-c" }],
      customCss: ["./assets/css/fix-overview.css", "./assets/css/fixed-bugs.css", "./src/styles/fixed-bugs.css", "./src/styles/fix-overview.css"],
      components: { Head: "./src/components/Head.astro", SiteTitle: "./src/components/SiteTitle.astro" },
      sidebar: sidebar(fileURLToPath(new URL("./src/content/docs", import.meta.url))),
      // the Blitz Basic grammar (src/lib/grammar.mjs: blitzbasic, bb2,
      // blitz2), also for basic: the fragments of test programs on the test
      // pages, which are Blitz sources too
      expressiveCode: {
        shiki: {
          langs: [blitzbasicGrammar(fileURLToPath(new URL("./data", import.meta.url)))],
          langAlias: { basic: "blitzbasic" },
        },
      },
    }),
  ],
});
