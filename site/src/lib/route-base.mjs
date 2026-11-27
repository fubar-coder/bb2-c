// Starlight route middleware of the site (site-starlight, design D7; added
// by src/integrations/site-checks.mjs): the base on the links of the front
// matter, which Starlight uses as written. The pages keep site paths
// without the base ("/docs/a/"), as in their Markdown links:
//
// * prev/next ({link, label}): the pager's link, where it is the page's;
// * the hero's buttons (hero.actions[].link) of the home page.
import { defineRouteMiddleware } from "@astrojs/starlight/route-data";

const BASE = import.meta.env.BASE_URL.replace(/\/$/, "");
const withBase = (href) => (typeof href === "string" && href.startsWith("/") && !href.startsWith("//") ? BASE + href : href);

export const onRequest = defineRouteMiddleware((context) => {
  const route = context.locals.starlightRoute;
  const data = route.entry.data;
  for (const k of ["prev", "next"]) {
    const link = route.pagination[k];
    if (link && data[k]?.link !== undefined && link.href === data[k].link) link.href = withBase(link.href);
  }
  for (const action of data.hero?.actions ?? []) action.link = withBase(action.link);
});
