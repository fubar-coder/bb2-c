// The content collections of the Starlight site: the docs collection with
// Starlight's loader (src/content/docs/; it skips files whose name begins
// with "_") and the i18n collection with Starlight's UI strings.
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

export const collections = {
  docs: defineCollection({
    loader: docsLoader(),
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
