// The content collections of the Starlight site: the docs collection with
// Starlight's loader (src/content/docs/; it skips files whose name begins
// with "_") and the pages of the command reference, and the i18n collection
// with Starlight's UI strings.
//
// The command reference (src/lib/cmdref.mjs, the port of Hugo's content
// adapter content/docs/libraries/_content.gotmpl) adds one entry per
// library, rendered with the site's Markdown pipeline (the remark plugin
// included, which gets the entry's path as its file). The entries carry a
// filePath below src/content/docs/ that has no file, as Starlight takes an
// entry's place from it; src/lib/sidebar.mjs lists them in the sidebar.
//
// The docs schema is Starlight's with the front matter the pages keep from
// Hugo (scripts/convert-hugo.mjs): aliases (the old URLs of the page, for
// the redirect pages of src/lib/redirects.mjs), anchorMoves (heading id ->
// the page it moved to, below /docs/compared-with-original/) and width
// ("wide": the page uses the full width).
//
// Starlight reads the i18n collection on every page, and Astro warns for a
// collection that has no entry (an undeclared one or an empty glob alike),
// so the collection holds en.json, the overrides of Starlight's English UI
// strings (none yet).
import { defineCollection } from "astro:content";
import { z } from "astro/zod";
import { docsLoader, i18nLoader } from "@astrojs/starlight/loaders";
import { docsSchema, i18nSchema } from "@astrojs/starlight/schema";
import { referencePages } from "./lib/cmdref.mjs";

/** Starlight's loader of the page files, then the pages of the command reference */
function docsWithCommandReference() {
  const files = docsLoader();
  return {
    name: "bb2-docs",
    async load(context) {
      // Astro keeps the rendered pages between builds (node_modules/.astro)
      // and renders a page file again only when the file changes; the pages
      // also show site/data and the remark plugin's output, so every build
      // renders every page
      context.store.clear();
      await files.load(context);
      for (const page of referencePages()) {
        const filePath = `src/content/docs/${page.id}.md`;
        const data = await context.parseData({
          id: page.id,
          data: { title: page.title, description: page.description },
          filePath,
        });
        context.store.set({
          id: page.id,
          data,
          body: page.markdown,
          filePath,
          digest: context.generateDigest(page.markdown),
          rendered: await context.renderMarkdown(page.markdown, { fileURL: new URL(filePath, context.config.root) }),
        });
      }
    },
  };
}

export const collections = {
  docs: defineCollection({
    loader: docsWithCommandReference(),
    schema: docsSchema({
      extend: z.object({
        aliases: z.array(z.string()).optional(),
        anchorMoves: z.record(z.string(), z.string()).optional(),
        width: z.enum(["wide"]).optional(),
      }),
    }),
  }),
  i18n: defineCollection({ loader: i18nLoader(), schema: i18nSchema() }),
};
