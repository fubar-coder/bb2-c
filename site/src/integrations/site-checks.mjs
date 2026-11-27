// The site's Starlight plugin (site-starlight, design D7, D9): the base on
// the links that are no Markdown links, and the checks of the build.
//
// * The route middleware src/lib/route-base.mjs adds the base to the links
//   of the front matter, which Starlight uses as written: prev/next and the
//   hero's buttons. (The Markdown links and the cards get it from the
//   site's remark plugin, src/plugins/remark-bb2.mjs.)
// * The leftover-syntax check, before the build: a shortcode of Hugo
//   (`{{<`, `{{%`) outside a code block in a page (src/content/docs/) or a
//   data file (data/) fails the build, naming the file and the line. Until
//   the cut-over the data link with Hugo's relref, which src/lib/data.mjs
//   resolves, so the check runs only with BB2_HUGO_SYNTAX_CHECK=1; the
//   cut-over (task 9.1) converts the data and turns it on for good. The
//   pages are converted from Hugo's at every build until then, and the
//   converter fails on a shortcode it leaves.
// * The check of the overview of fixed bugs, after the build
//   (src/lib/fix-overview.mjs; first, so that a row's broken anchor is
//   named by its row, not as a broken link): its data against the pages of
//   fixed bugs, every problem named with its row or issue.
// * The link check, after the build, over the output (src/lib/links.mjs):
//   a broken internal link of a page or of the data fails the build,
//   naming the page or the data file and the link.
// * Then the old URLs (src/lib/redirects.mjs): the redirect pages of the
//   pages' aliases, and the check that every URL of tests/urls/*.txt (the
//   Hugo build's) is in the output; a broken anchorMoves or a missing URL
//   fails the build, naming the page or the URL.
//
// A failed check removes the output directory, so that no site is left
// behind, and throws: `astro build` exits non-zero with the messages.
//
// The integration also defines __BB2_SITE__, the site directory, for
// src/lib/data.mjs in the bundled page code.
import { readdirSync, readFileSync, rmSync, statSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import { segments, lineOf } from "../lib/markdown.mjs";
import { checkLinks } from "../lib/links.mjs";
import { checkFixOverview } from "../lib/fix-overview.mjs";
import { writeRedirects, checkUrls } from "../lib/redirects.mjs";

/** Every file below dir, as paths relative to dir with "/" */
function walk(dir) {
  return readdirSync(dir).flatMap((name) => {
    const p = join(dir, name);
    return statSync(p).isDirectory() ? walk(p).map((f) => `${name}/${f}`) : [name];
  });
}

/** The places of Hugo's shortcode syntax outside code blocks: ["FILE:LINE: text"] */
export function hugoSyntax(site) {
  const files = [
    ...walk(join(site, "src", "content", "docs"))
      .filter((f) => /\.mdx?$/.test(f))
      .map((f) => `src/content/docs/${f}`),
    ...walk(join(site, "data"))
      .filter((f) => /\.(ya?ml|json)$/.test(f))
      .map((f) => `data/${f}`),
  ];
  const found = [];
  for (const file of files) {
    const src = readFileSync(join(site, file), "utf8");
    let off = 0;
    for (const s of segments(src)) {
      if (!s.code)
        for (const m of s.text.matchAll(/\{\{[<%][^\n]{0,60}/g))
          found.push(`${file}:${lineOf(src, off + m.index)}: ${m[0]}`);
      off += s.text.length;
    }
  }
  return found;
}

/** Removes the output and throws with the messages (the first limit of them shown) */
function fail(outDir, what, messages, logger, limit = 100) {
  rmSync(outDir, { recursive: true, force: true });
  const shown = messages.slice(0, limit);
  const more = messages.length > shown.length ? `\n... and ${messages.length - shown.length} more` : "";
  const text = `${what} (${messages.length}):\n${shown.join("\n")}${more}`;
  logger.error(text);
  throw new Error(`${what}; the output ${outDir} is removed`);
}

/** The redirect pages of the aliases and the check of the old URLs (src/lib/redirects.mjs) */
function oldUrls(site, dist, origin, base, logger) {
  const redirects = writeRedirects(join(site, "src", "content", "docs"), dist, origin, base);
  if (redirects.errors.length) fail(dist, "broken aliases or anchorMoves", redirects.errors, logger);
  const { errors, urls, awaited } = checkUrls(join(site, "tests", "urls"), dist);
  if (errors.length) fail(dist, "old URLs missing from the output", errors, logger);
  const waiting = Object.entries(awaited).map(([task, n]) => `${n} of task ${task}`);
  logger.info(`${redirects.written} redirect pages written; ${urls} old URLs checked${waiting.length ? `; not yet built: ${waiting.join(", ")}` : ""}`);
}

export function siteChecks() {
  return {
    name: "bb2-site-checks",
    hooks: {
      "config:setup"({ addIntegration, addRouteMiddleware }) {
        addRouteMiddleware({ entrypoint: fileURLToPath(new URL("../lib/route-base.mjs", import.meta.url)) });
        let site;
        let outDir;
        let base;
        let origin;
        addIntegration({
          name: "bb2-site-checks",
          hooks: {
            "astro:config:setup"({ config, updateConfig }) {
              updateConfig({ vite: { define: { __BB2_SITE__: JSON.stringify(fileURLToPath(config.root)) } } });
            },
            "astro:config:done"({ config }) {
              site = fileURLToPath(config.root);
              origin = config.site.replace(/\/$/, "");
              outDir = fileURLToPath(config.outDir);
              base = config.base.replace(/\/?$/, "/");
            },
            "astro:build:start"({ logger }) {
              if (process.env.BB2_HUGO_SYNTAX_CHECK !== "1") return;
              const found = hugoSyntax(site);
              if (found.length) fail(outDir, "Hugo's shortcode syntax outside a code block", found, logger);
            },
            "astro:build:done"({ dir, logger }) {
              const dist = fileURLToPath(dir);
              const overview = checkFixOverview(site, dist, base);
              if (overview.length) fail(dist, "the overview of fixed bugs does not match its pages", overview, logger, Infinity);
              const { errors, links, awaited } = checkLinks(dist, base);
              if (errors.length) fail(dist, "broken internal links", errors, logger);
              const waiting = Object.entries(awaited).map(([task, n]) => `${n} to targets of task ${task}`);
              logger.info(`${links} internal links checked${waiting.length ? `; not yet built: ${waiting.join(", ")}` : ""}`);
              oldUrls(site, dist, origin, base, logger);
            },
          },
        });
      },
    },
  };
}

