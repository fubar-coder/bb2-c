// Starlight route middleware of the site (site-starlight, design D7; added
// by src/integrations/site-checks.mjs).
//
// The base on the links of the front matter, which Starlight uses as
// written. The pages keep site paths without the base ("/docs/a/"), as in
// their Markdown links:
//
// * prev/next ({link, label}): the pager's link, where it is the page's;
// * the hero's buttons (hero.actions[].link) of the home page.
//
// The pager's labels: Starlight takes a neighbour's label from the sidebar,
// where a section's own page is "Overview" (src/lib/sidebar.mjs); the pager
// names the page by its title, as Hugo's did. A label of the front matter's
// prev/next stays.
import { getCollection } from "astro:content";
import { defineRouteMiddleware } from "@astrojs/starlight/route-data";

const BASE = import.meta.env.BASE_URL.replace(/\/$/, "");
const withBase = (href) => (typeof href === "string" && href.startsWith("/") && !href.startsWith("//") ? BASE + href : href);

/** The title of each page by its path with the base ("/bb2-c/docs/a/") */
let titles;
const pageTitles = async () =>
  (titles ??= new Map((await getCollection("docs")).map((e) => [`${BASE}/${e.id === "index" ? "" : `${e.id}/`}`, e.data.title])));

export const onRequest = defineRouteMiddleware(async (context) => {
  const route = context.locals.starlightRoute;
  const data = route.entry.data;
  for (const k of ["prev", "next"]) {
    const link = route.pagination[k];
    if (!link) continue;
    if (data[k]?.link !== undefined && link.href === data[k].link) link.href = withBase(link.href);
    if (typeof data[k] === "string" || data[k]?.label !== undefined) continue;
    const title = (await pageTitles()).get(link.href);
    if (title !== undefined) link.label = title;
  }
  for (const action of data.hero?.actions ?? []) action.link = withBase(action.link);
});
