// The TextMate grammar of Blitz Basic 2 (scope source.blitzbasic) for the
// code blocks of the Starlight site (site-starlight, design D11),
// registered with Expressive Code's Shiki in astro.config.mjs; fence
// languages blitzbasic, bb2 and blitz2.
//
// The names come from the site's data, none is listed here but the
// operator words below:
//
// * the compiler's keywords: data/compiler/blitz2.json, the TK entries of
//   the compiler's token list (programs/blitz2/src/toklist.h), written and
//   checked by tools/toklist in blitz2's build step;
// * the commands, functions and statements and the objects of the
//   libraries: data/libraries/acidlibs and userlibs; the AmigaOS calls:
//   data/libraries/deflibs (blitzlib cmdref, checked in each library's
//   build step).
//
// The operators of an expression that are words: AND, OR, EOR, NOT, LSL,
// LSR, ASL, ASR are nodes of the token list's assembler mnemonics (TA),
// which the export leaves out, and the compiler's expression evaluator
// takes them as operators (programs/blitz2/src/eval.c, the operator table);
// Mod and the bit test operators are keywords of the export, marked as
// operators here.
import { readFileSync, readdirSync } from "node:fs";
import { join } from "node:path";

/** The operator words that are no keyword of the export (TA nodes) */
const ASM_OPERATORS = ["AND", "OR", "EOR", "NOT", "LSL", "LSR", "ASL", "ASR"];
/** The keywords of the export that are operators */
const KEYWORD_OPERATORS = ["MOD", "BitTst", "BitSet", "BitClr", "BitChg"];

const readJson = (file) => JSON.parse(readFileSync(file, "utf8"));

/** The entries of the library files of a group (data/libraries/GROUP/*.json) */
function libraryEntries(dataDir, group) {
  const dir = join(dataDir, "libraries", group);
  return readdirSync(dir)
    .filter((f) => f.endsWith(".json"))
    .sort()
    .flatMap((f) => readJson(join(dir, f)).entries);
}

const reEscape = (s) => s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
// the names without case (as the compiler and the editor take them),
// longest first, so that "Left$" wins over "Left"
function alternation(names) {
  const seen = new Map();
  for (const n of names) if (!seen.has(n.toLowerCase())) seen.set(n.toLowerCase(), n);
  return [...seen.values()]
    .sort((a, b) => b.length - a.length || (a < b ? -1 : a > b ? 1 : 0))
    .map(reEscape)
    .join("|");
}

/**
 * The grammar, from the data of the site in DATADIR (site/data)
 * @param {string} dataDir
 */
export function blitzbasicGrammar(dataDir) {
  const compiler = readJson(join(dataDir, "compiler", "blitz2.json"));
  const tokens = compiler.keywords.map((k) => k.name);
  const lower = new Set(tokens.map((t) => t.toLowerCase()));
  for (const op of KEYWORD_OPERATORS) {
    if (!lower.has(op.toLowerCase())) {
      throw new Error(`grammar: the operator ${op} is no keyword of data/compiler/blitz2.json`);
    }
  }
  const operatorWords = new Set([...ASM_OPERATORS, ...KEYWORD_OPERATORS].map((o) => o.toLowerCase()));
  const keywords = tokens.filter((t) => !operatorWords.has(t.toLowerCase()));

  const commands = [];
  const objects = [];
  for (const group of ["acidlibs", "userlibs"]) {
    for (const e of libraryEntries(dataDir, group)) (e.kind === "object" ? objects : commands).push(e.name);
  }
  const oscalls = libraryEntries(dataDir, "deflibs").map((e) => e.name);

  // a name stands alone: no letter, digit, "_" or "$" (or the "." of a
  // label or type) directly before it, no letter, digit or "_" after it
  const word = (alt) => `(?i)(?<![\\w$.])(?:${alt})(?![\\w])`;
  return {
    name: "blitzbasic",
    scopeName: "source.blitzbasic",
    displayName: "Blitz Basic 2",
    aliases: ["bb2", "blitz2"],
    patterns: [
      { include: "#comment" },
      { include: "#string" },
      { include: "#number" },
      { include: "#macro-call" },
      { include: "#keyword" },
      { include: "#operator-word" },
      { include: "#oscall" },
      { include: "#command" },
      { include: "#object" },
      { include: "#constant" },
      { include: "#label" },
      { include: "#typed-name" },
      { include: "#operator" },
    ],
    repository: {
      comment: { name: "comment.line.semicolon.blitzbasic", match: ";.*$" },
      string: { name: "string.quoted.double.blitzbasic", begin: '"', end: '"|$' },
      number: {
        patterns: [
          { name: "constant.numeric.hex.blitzbasic", match: "\\$[0-9A-Fa-f]+" },
          { name: "constant.numeric.binary.blitzbasic", match: "%[01]+" },
          {
            name: "constant.numeric.decimal.blitzbasic",
            match: "(?<![\\w$.])(?:\\d+\\.?\\d*|\\.\\d+)(?:[eE][-+]?\\d+)?",
          },
        ],
      },
      // !name{...}: the call of a macro
      "macro-call": { name: "entity.name.function.macro.blitzbasic", match: "![A-Za-z_]\\w*" },
      keyword: { name: "keyword.control.blitzbasic", match: word(alternation(keywords)) },
      "operator-word": {
        name: "keyword.operator.word.blitzbasic",
        match: word(alternation([...ASM_OPERATORS, ...KEYWORD_OPERATORS])),
      },
      // an AmigaOS call ends with "_", so nothing may follow but a non-word
      oscall: { name: "support.function.amigaos.blitzbasic", match: word(alternation(oscalls)) },
      command: { name: "support.function.blitzbasic", match: word(alternation(commands)) },
      object: { name: "support.class.blitzbasic", match: word(alternation(objects)) },
      constant: { name: "constant.other.blitzbasic", match: "#[A-Za-z_]\\w*" },
      label: { name: "entity.name.label.blitzbasic", match: "^\\s*\\.?[A-Za-z_]\\w*:(?!=)" },
      "typed-name": {
        match: "([A-Za-z_]\\w*)(\\.(?:[bwlqfs]|[A-Za-z_]\\w*)\\b|\\$)",
        captures: {
          1: { name: "variable.other.blitzbasic" },
          2: { name: "storage.type.suffix.blitzbasic" },
        },
      },
      operator: { name: "keyword.operator.blitzbasic", match: "[-+*/^&|<>=@~]|\\\\" },
    },
  };
}
