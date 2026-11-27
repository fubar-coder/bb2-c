// The last-modified date of a page from git, for the meta tag
// article:modified_time (site-starlight, design D13): written in the CI
// build only, BB2_SITE_GITINFO=1, as Hugo's --enableGitInfo did (Hextra's
// opengraph partial); the date is not shown on the page. Without the
// variable the build runs no git, so a local build and the tests need none;
// with it a missing git or history fails the build.
//
// Hugo's rules: the date is the author date of the newest commit (no merge
// commits, no rename tracking) that changed the page's file, in the
// commit's time zone; a page without a file (the command reference) and a
// section's index page get no tag (Hugo writes it for regular pages only),
// nor does the 404 page (a draft entry; Hugo's is no page of the content).
// A section's index page is an index.md(x) with other pages of the
// collection below its directory (the command reference's index pages
// have only loader entries there); a page with its images
// (freqlib/index.md) is a regular page.
//
// Until the cut-over (task 9.1) the converted pages are not in git: the
// date is that of the Hugo page they come from (content/..., _index.md for
// an index page); task 9.2 removes that fallback with content/.
import { execFileSync } from "node:child_process";
import { existsSync } from "node:fs";
import { join } from "node:path";
import { SITE } from "./data.mjs";

export const GITINFO = process.env.BB2_SITE_GITINFO === "1";

let dates;
/** file path relative to the site -> ISO 8601 date of its newest commit */
function gitDates() {
  if (dates) return dates;
  let out;
  try {
    out = execFileSync(
      "git",
      ["-c", "diff.renames=0", "-c", "log.showSignature=0", "log", "--no-merges", "--relative", "--name-only", "--format=%x1e%aI", "--", "."],
      { cwd: SITE, encoding: "utf8", maxBuffer: 1 << 28 },
    );
  } catch (e) {
    throw new Error(`BB2_SITE_GITINFO=1, but git log failed in ${SITE}: ${e.message}`);
  }
  dates = new Map();
  for (const commit of out.split("\x1e").slice(1)) {
    const [date, ...files] = commit.split("\n").filter((l) => l !== "");
    for (const f of files) if (!dates.has(f)) dates.set(f, date);
  }
  if (dates.size === 0) throw new Error(`BB2_SITE_GITINFO=1, but git has no history of ${SITE}`);
  return dates;
}

/**
 * The page's modified date (ISO 8601), or null. file: the entry's filePath
 * (relative to the site); files: those of all entries of the collection.
 */
export function modifiedTime(file, files) {
  if (!GITINFO || !file || !existsSync(join(SITE, file))) return null;
  // a section's index page: an index.md(x) with other pages below its directory
  const dir = file.replace(/[^/]*$/, "");
  if (/(^|\/)index\.mdx?$/.test(file) && files.some((f) => f !== file && f.startsWith(dir))) return null;
  const d = gitDates();
  const hugo = file.replace(/^src\/content\/docs\//, "content/").replace(/\.mdx$/, ".md");
  for (const f of [file, hugo, hugo.replace(/(^|\/)index\.md$/, "$1_index.md")]) if (d.has(f)) return d.get(f);
  return null;
}
