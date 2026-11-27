// The website with Astro Starlight (site-starlight; Hugo, hugo.yaml, stays
// the published site until the cut-over). Built with Node.js in the pinned
// image docker.io/library/node:26.13.1-bookworm-slim via podman:
//
//   podman run --rm -v $PWD/site:/site -w /site \
//       docker.io/library/node:26.13.1-bookworm-slim sh -c 'npm ci && npm run build'
//
// The output is dist/; the build must finish without errors or warnings
// (ctest site_starlight). Astro's telemetry is off in the package scripts.
import { defineConfig } from "astro/config";
import starlight from "@astrojs/starlight";
import { unified } from "@astrojs/markdown-remark";
import { remarkBb2 } from "./src/plugins/remark-bb2.mjs";

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
  integrations: [
    starlight({
      title: "BB2 C port",
      social: [{ icon: "github", label: "GitHub", href: "https://github.com/fubar-coder/bb2-c" }],
      customCss: ["./assets/css/fix-overview.css", "./assets/css/fixed-bugs.css"],
      // blitzbasic: plain text until the Blitz Basic grammar (task 8.1);
      // basic: the fragments of test programs on the test pages, plain text
      // as in Hugo
      expressiveCode: {
        shiki: { langAlias: { blitzbasic: "plaintext", basic: "plaintext" } },
      },
    }),
  ],
});
