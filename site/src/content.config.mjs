// The content collections of the Starlight site: the docs collection with
// Starlight's loader (src/content/docs/; it skips files whose name begins
// with "_") and the i18n collection with Starlight's UI strings.
//
// Starlight reads the i18n collection on every page, and Astro warns for a
// collection that has no entry (an undeclared one or an empty glob alike),
// so the collection holds en.json, the overrides of Starlight's English UI
// strings (none yet).
import { defineCollection } from "astro:content";
import { docsLoader, i18nLoader } from "@astrojs/starlight/loaders";
import { docsSchema, i18nSchema } from "@astrojs/starlight/schema";

export const collections = {
  docs: defineCollection({ loader: docsLoader(), schema: docsSchema() }),
  i18n: defineCollection({ loader: i18nLoader(), schema: i18nSchema() }),
};
