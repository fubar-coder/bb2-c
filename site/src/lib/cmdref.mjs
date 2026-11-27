// The command reference (site-starlight, task 5.2, design D8): one page per
// command library (acidlibs), RWE userlib and AmigaOS library or resource,
// made from site/data (src/lib/data.mjs). The port of Hugo's content adapter
// content/docs/libraries/_content.gotmpl and its partials
// layouts/_partials/cmdref/{commands,entry,form,arg,escape,fixes,oscalls}.html;
// the loader of src/content.config.mjs adds the pages to the docs
// collection, src/lib/sidebar.mjs lists them.
//
// * data/libraries/GROUP/NAME.json: the commands of a library;
// * data/libintros/GROUP/NAME.yaml: "description" (one sentence, for the
//   lists and search) and "intro" (Markdown above the commands);
// * data/cmddesc/GROUP/NAME.yaml: per command "text" (under its heading) and
//   "notes" (Markdown after its forms);
// * data/fixoverview.yaml: the fixed original bugs of a library, shown with
//   the commands their row names and the others in a section of their own.
//
// The Markdown links with site paths, to which the site's remark plugin adds
// the base. The headings of the commands carry Hugo's ids ({#id}, entryIds),
// which the links of the pages of fixed bugs (src/lib/fixed-bugs.mjs) name;
// the call tables of the AmigaOS pages get their break opportunities from
// the remark plugin (the port of render-table.html).
import { byText, cmddesc, entryIds, fixoverview, libintros, libraries } from "./data.mjs";
import { commandName, levelBadge, rowHref, splitList } from "./fixed-bugs.mjs";

const notation = { byte: ".b", word: ".w", long: ".l", quick: ".q", float: ".f", string: "$", any: "any", suffix: ".Type", array: "array()" };
const resultNotation = { byte: ".b", word: ".w", long: ".l", quick: ".q", float: ".f", string: "$", any: "any" };
const kinds = { statement: "Statement", function: "Function", command: "Statement or function", object: "Object name", keyword: "Keyword" };

/** A name as Markdown text, the characters of Markdown markup escaped (cmdref/escape.html) */
const escapeMd = (s) => s.replace(/[\\`*_[\]<>#|]/g, (c) => `\\${c}`);

/** One argument in Blitz notation (cmdref/arg.html) */
function arg(a) {
  const s = notation[a.type] ?? a.type;
  return (a.flags ?? []).includes("address") ? `&${s}` : s;
}

/** The arguments of a form, a repetition group as "{ ... }..." (cmdref/form.html) */
function form(f) {
  const args = (f.args ?? []).map(arg);
  if (!args.length) return "no arguments";
  if (!f.repeat) return "`" + args.join(", ") + "`";
  const from = Number(f.repeat.from);
  const count = Number(f.repeat.count);
  const parts = [];
  if (from > 0) parts.push(args.slice(0, from).join(", "));
  parts.push(`{ ${args.slice(from, from + count).join(", ")} }...`);
  if (args.length > from + count) parts.push(args.slice(from + count).join(", "));
  return "`" + parts.join(", ") + "`";
}

/** The Markdown list of fixed original bugs, by issue (cmdref/fixes.html) */
const fixes = (rows) =>
  [...rows]
    .sort((a, b) => a.issue - b.issue)
    .map((r) => `- [${r.text.trim()}](${rowHref(r)}) <span class="fixed-bug-meta">${levelBadge(r.severity)}</span>\n`)
    .join("");

/** One entry of a command library (cmdref/entry.html) */
function entry(e, id, desc, rows) {
  const facts = [kinds[e.kind] ?? e.kind];
  if (e.typed) facts.push("takes a type suffix such as `.w`");
  if (e.result) facts.push(e.result === "suffix" ? "result of the type of the suffix" : `result \`${resultNotation[e.result] ?? e.result}\``);
  let md = `### ${escapeMd(e.name)} {#${id}}\n\n`;
  if (desc?.text) md += `${desc.text.trim()}\n\n`;
  md += `${facts.join(", ")}.`;
  md += e.help ? ` Help text: \`${e.help}\`` : " No help text.";
  const forms = e.forms ?? [];
  if (forms.length === 1 && !(forms[0].args ?? []).length) md += "\\\nNo arguments.\n\n";
  else if (forms.length === 1) md += `\\\nArguments: ${form(forms[0])}\n\n`;
  else if (forms.length) md += `\\\n${forms.length} forms:\n\n${forms.map((f) => `- ${form(f)}\n`).join("")}\n`;
  else md += "\n\n";
  if (desc?.notes) md += `${desc.notes.trim()}\n\n`;
  if (rows?.length) md += `Fixed original bugs:\n\n${fixes(rows)}\n`;
  return md;
}

/** The page of a command library or userlib (cmdref/commands.html) */
function commandsMarkdown(lib, intro, descs, libFixes, fixpage) {
  let md = intro ? `${intro}\n\n` : "";
  let facts = `Library number ${lib.number}`;
  if (lib.object) facts += `. It manages \`${lib.object.name}\` objects, by default at most ${lib.object.maximum} (the compiler options set the maximum)`;
  // the fixes of a command: the rows whose "command" names it
  const bycmd = {};
  const matched = new Set();
  for (const e of lib.entries) {
    const name = e.name.toLowerCase();
    const rows = libFixes.filter((r) => splitList(r.command).some((c) => commandName(c).toLowerCase() === name));
    for (const r of rows) matched.add(r.issue);
    if (rows.length) bycmd[name] = [...new Set(rows)];
  }
  const other = libFixes.filter((r) => !matched.has(r.issue));
  const hasBycmd = Object.keys(bycmd).length > 0;
  let fixline = "";
  const n = new Set(libFixes).size;
  if (n) {
    let where = hasBycmd ? `below with the ${n === 1 ? "command it concerns" : "commands they concern"}` : "at the end of this page";
    if (libFixes.some((r) => r.page === fixpage))
      where += ` and on the page [${fixpage.split("/").pop()}](/docs/compared-with-original/${fixpage}/) of the fixed original bugs`;
    fixline = `The BB2 C port fixes ${n} ${n === 1 ? "bug" : "bugs"} of the original in this library, listed ${where}.`;
  }
  if (lib.entries.length) {
    md += `${facts}.\n\n`;
    if (fixline) md += `${fixline}\n\n`;
    md += "## Commands\n\n";
    for (const { entry: e, id } of entryIds(lib.entries)) md += entry(e, id, descs[e.name], bycmd[e.name.toLowerCase()]);
  } else {
    md += `${facts}.\n\nThis library declares no commands. Its routines are used by the compiler, by the code it generates or by other libraries.\n`;
    if (fixline) md += `\n${fixline}\n`;
  }
  if (other.length)
    md += `\n## Fixed original bugs\n\n${hasBycmd ? "The fixes of the library that concern no single command above:" : "The fixes of the library:"}\n\n${fixes(other)}`;
  return md;
}

/** The page of an AmigaOS library (cmdref/oscalls.html) */
function oscallsMarkdown(calls, numbers, intro, osFixes) {
  let md = intro ? `${intro}\n\n` : "";
  const list = numbers.length > 1 ? `${numbers.slice(0, -1).join(", ")} and ${numbers.at(-1)}` : `${numbers[0]}`;
  md += `Declared by deflibs ${numbers.length > 1 ? "libraries" : "library"} ${list}.\n\n`;
  md += "## Calls\n\n| Call | Arguments | Registers | Offset |\n|---|---|---|---:|\n";
  for (const c of [...calls].sort((a, b) => byText(a.name, b.name))) {
    const args = c.help ? `\`${c.help.replaceAll("|", "\\|")}\`` : "";
    md += `| \`${c.name}\` | ${args} | ${(c.registers ?? []).join(", ")} | ${c.offset} |\n`;
  }
  if (osFixes.length)
    md += `\n## Fixed original bugs\n\nThe BB2 C port fixes these bugs of the original in the calls of this library ([AmigaOS calls](/docs/compared-with-original/libraries/amigaos/)):\n\n${fixes(osFixes)}`;
  return md;
}

/** The reference sections: the group of site/data and the directory of its pages */
export const SECTIONS = { commands: "acidlibs", userlibs: "userlibs", amigaos: "deflibs" };

/**
 * The pages of the command reference: [{id, section, name, title,
 * description, number, count, about, markdown}], id the docs entry
 * ("docs/libraries/commands/windowslib"), about the hand-written
 * description or "". Cached: the data do not change during a build.
 */
let pages;
export function referencePages() {
  if (pages) return pages;
  const L = libraries();
  const intros = libintros();
  const descs = cmddesc();
  // the fixed original bugs of each library: a row concerns every library
  // its "library" names; the rows of the page of the AmigaOS calls are of
  // deflibs libraries, the others of command libraries and userlibs
  const byLib = {};
  const byOsLib = {};
  for (const row of fixoverview().rows)
    for (const name of splitList(row.library)) ((row.page === "libraries/amigaos" ? byOsLib : byLib)[name] ??= []).push(row);

  const out = [];
  for (const [section, group] of [
    ["commands", "acidlibs"],
    ["userlibs", "userlibs"],
  ])
    for (const [name, lib] of Object.entries(L[group])) {
      const intro = intros[group]?.[name] ?? {};
      const n = lib.entries.filter((e) => ["statement", "function", "command"].includes(e.kind)).length;
      out.push({
        id: `docs/libraries/${section}/${name}`,
        section,
        name,
        title: name,
        description: intro.description || `The ${n} commands of ${name}, with their forms and argument types.`,
        number: lib.number,
        count: n,
        about: intro.description ?? "",
        markdown: commandsMarkdown(lib, intro.intro, descs[group]?.[name] ?? {}, byLib[name] ?? [], `libraries/${section}/${name}`),
      });
    }

  // AmigaOS libraries (deflibs): one page per library or resource that a
  // deflibs library opens, with the calls of every deflibs library that
  // uses its base, the libraries by number from the highest
  const deflibs = Object.values(L.deflibs).sort((a, b) => b.number - a.number);
  for (const [name, lib] of Object.entries(L.deflibs)) {
    if (!lib.opens) continue;
    const intro = intros.deflibs?.[name] ?? {};
    const calls = [];
    const numbers = [];
    for (const d of deflibs) {
      const own = d.entries.filter((e) => e.base === lib.number);
      if (own.length) {
        calls.push(...own);
        numbers.push(d.number);
      }
    }
    out.push({
      id: `docs/libraries/amigaos/${name}`,
      section: "amigaos",
      name,
      title: lib.opens,
      description: intro.description || `The ${calls.length} calls of ${lib.opens} that a program can use directly, with their registers.`,
      number: lib.number,
      count: calls.length,
      about: intro.description ?? "",
      markdown: oscallsMarkdown(calls, numbers, intro.intro, byOsLib[name] ?? []),
    });
  }
  pages = out;
  return out;
}

/** The reference pages of a section ("commands"), by title as Hugo lists them */
export const sectionPages = (section) => referencePages().filter((p) => p.section === section).sort((a, b) => byText(a.title, b.title));
