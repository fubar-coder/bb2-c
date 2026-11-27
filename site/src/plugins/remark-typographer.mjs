// The typography of Hugo's Markdown (site-starlight, design D4; findings of
// group 8): Hugo's Goldmark has its typographer on by default, which turns
// "..." into “…”, '...' into ‘…’, an apostrophe into ’, -- into –, --- into
// —, ... into …, << and >> into « and ». Astro's smartypants (retext-
// smartypants) decides some quotes otherwise ("" and "+" next to
// punctuation) and makes -- an em dash, so it is off (astro.config.mjs) and
// this plugin does what Goldmark does: a port of the inline parser of
// Goldmark v1.8.5 (the version in Hugo 0.165.0), extension/typographer.go,
// with the delimiter scan of parser/delimiter.go.
//
// Goldmark decides on the source line: the character before a quote and
// the rest of the line after it, with the raw text of the inline elements
// (code spans, links, emphasis, inline HTML) in between, and one count of
// open quotes per block. The plugin builds that line per paragraph,
// heading and table cell from the Markdown tree: the text of the text
// nodes, which it may change, and a stand-in for every other inline node
// (`code`, `*`, `[`...`](url)`, the HTML), which it does not change. An
// escaped character (`\-`) or an entity keeps its source form, as Goldmark
// sees it, when the text node's position points at its source. The
// line is UTF-8 bytes, as in Goldmark, whose length and index tests are on
// bytes.
//
// Where Hextra renders a shortcode's text with markdownify, it gets the
// typography too: a file name of the filetree (Starlight's FileTree, a list
// here) and the card's subtitle (LinkCard's description); a folder name of
// the filetree, which Hextra writes as it is, gets none.
//
// The port follows Goldmark, which is under the MIT licence:
//
//   Copyright (c) 2019 Yusuke Inuzuka
//
//   Permission is hereby granted, free of charge, to any person obtaining a
//   copy of this software and associated documentation files (the
//   "Software"), to deal in the Software without restriction, including
//   without limitation the rights to use, copy, modify, merge, publish,
//   distribute, sublicense, and/or sell copies of the Software, and to
//   permit persons to whom the Software is furnished to do so, subject to
//   the following conditions:
//
//   The above copyright notice and this permission notice shall be included
//   in all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
//   NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
//   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
//   USE OR OTHER DEALINGS IN THE SOFTWARE.

const LSQUO = "‘";
const RSQUO = "’";
const LDQUO = "“";
const RDQUO = "”";
const NDASH = "–";
const MDASH = "—";
const HELLIP = "…";
const LAQUO = "«";
const RAQUO = "»";
const APOS = RSQUO;

const ch = (s) => s.charCodeAt(0);
const [SQ, DQ, MINUS, DOT, LT, GT, NL, BSL, AMP] = ["'", '"', "-", ".", "<", ">", "\n", "\\", "&"].map(ch);
// Goldmark's trigger characters (',', '*' and '[' come to nothing)
const TRIGGER = new Set([SQ, DQ, MINUS, DOT, LT, GT]);

// util.IsPunct and util.IsSpace: ASCII bytes
const isPunctByte = (b) => (b >= 33 && b <= 47) || (b >= 58 && b <= 64) || (b >= 91 && b <= 96) || (b >= 123 && b <= 126);
const isSpaceByte = (b) => b === 9 || b === 10 || b === 13 || b === 32;
const isNumericByte = (b) => b >= 48 && b <= 57;
// Go's unicode classes, and util.IsPunctRune/IsSpaceRune
const isPunct = (r) => /\p{P}/u.test(r);
const isSpace = (r) => /\p{White_Space}/u.test(r);
const isLetter = (r) => /\p{L}/u.test(r);
const isDigit = (r) => /\p{Nd}/u.test(r);
const isPunctRune = (r) => /[\p{P}\p{S}]/u.test(r);
const isSpaceRune = isSpace;

/** util.ToRune: the character whose UTF-8 sequence holds byte pos */
function toRune(bytes, pos) {
  let i = pos;
  while (i > 0 && (bytes[i] & 0xc0) === 0x80) i--;
  const b = bytes[i];
  const n = b < 0x80 ? 1 : b < 0xe0 ? 2 : b < 0xf0 ? 3 : 4;
  return new TextDecoder().decode(bytes.subarray(i, i + n));
}

/** parser.ScanDelimiter for the quote at line[0] */
function scanDelimiter(line, before) {
  let j = 0;
  while (j < line.length && line[j] === line[0]) j++;
  const after = j !== line.length ? toRune(line, j) : " ";
  const beforePunct = isPunctRune(before);
  const beforeSpace = isSpaceRune(before);
  const afterPunct = isPunctRune(after);
  const afterSpace = isSpaceRune(after);
  return {
    canOpen: !afterSpace && (!afterPunct || beforeSpace || beforePunct),
    canClose: !beforeSpace && (!beforePunct || afterSpace || afterPunct),
  };
}

/**
 * typographerParser.Parse: the substitution at line[0] as [length in
 * bytes, text], or null. counter: the block's open quotes {single, double}.
 */
function parse(line, before, counter) {
  const n = line.length;
  const c = line[0];
  if (n > 2) {
    if (c === MINUS) {
      if (line[1] === MINUS && line[2] === MINUS) return [3, MDASH];
    } else if (c === DOT) {
      if (line[1] === DOT && line[2] === DOT) return [3, HELLIP];
      return null;
    }
  }
  if (n > 1) {
    if (c === LT) return line[1] === LT ? [2, LAQUO] : null;
    if (c === GT) return line[1] === GT ? [2, RAQUO] : null;
    if (c === MINUS && line[1] === MINUS) return [2, NDASH];
  }
  if (c !== SQ && c !== DQ) return null;
  const d = scanDelimiter(line, before);
  if (c === SQ) {
    // decade abbreviations such as '90s
    if (d.canOpen && !d.canClose && n > 3 && isNumericByte(line[1]) && isNumericByte(line[2]) && line[3] === ch("s")) {
      const after = n > 4 ? toRune(line, 4) : " ";
      if (n === 3 || isSpaceRune(after) || isPunctRune(after)) return [1, APOS];
    }
    // 'twas, 'em, 'net
    if (n > 1 && (isPunct(before) || isSpace(before)) && [ch("t"), ch("e"), ch("n"), ch("l")].includes(line[1]))
      return [1, APOS];
    // an apostrophe between two alphanumerics
    if (n > 1 && (isDigit(before) || isLetter(before)) && isLetter(toRune(line, 1))) return [1, APOS];
    if (d.canOpen && !d.canClose) {
      let right = false;
      // Alice's, I'm, Don't, You'd
      if (n > 1 && [ch("s"), ch("m"), ch("t"), ch("d")].includes(line[1]) && (n < 3 || isPunctByte(line[2]) || isSpaceByte(line[2])))
        right = true;
      // I've, I'll, You're
      if (
        n > 2 &&
        ((line[1] === ch("v") && line[2] === ch("e")) || (line[1] === ch("l") && line[2] === ch("l")) || (line[1] === ch("r") && line[2] === ch("e"))) &&
        (n < 4 || isPunctByte(line[3]) || isSpaceByte(line[3]))
      )
        right = true;
      if (!right) counter.single++;
      return [1, right ? RSQUO : LSQUO];
    }
    // Smiths', doin' (Goldmark tests line[0], the quote itself: so any
    // quote with two more bytes on the line and no digit after it)
    if ((n > 1 && isSpace(toRune(line, 0))) || (isPunct(toRune(line, 0)) && n > 2 && !isDigit(toRune(line, 1)))) return [1, RSQUO];
    if (counter.single > 0) {
      const isClose = d.canClose && !d.canOpen;
      const maybeClose =
        d.canClose && d.canOpen && n > 1 && isPunct(toRune(line, 1)) && (n === 2 || (n > 2 && isPunctByte(line[2])) || isSpaceByte(line[2]));
      if (isClose || maybeClose) {
        counter.single--;
        return [1, RSQUO];
      }
    }
    return null;
  }
  if (d.canOpen && !d.canClose) {
    counter.double++;
    return [1, LDQUO];
  }
  if (counter.double > 0) {
    const isClose = d.canClose && !d.canOpen;
    const maybeClose =
      d.canClose && d.canOpen && n > 1 && isPunct(toRune(line, 1)) && (n === 2 || (n > 2 && isPunctByte(line[2])) || isSpaceByte(line[2]));
    if (isClose || maybeClose) {
      // "Monitor 21""
      if (n > 1 && line[1] === DQ && isDigit(before)) return null;
      counter.double--;
      return [1, RDQUO];
    }
  }
  return null;
}

/**
 * The source line of a block, built from its inline nodes: bytes, and per
 * byte the text node it belongs to (or null: not to be changed) and the
 * index of its character in the node's value.
 */
class Line {
  constructor(source) {
    this.source = source;
    this.bytes = [];
    this.node = [];
    this.index = [];
  }
  raw(s) {
    for (const b of new TextEncoder().encode(s)) {
      this.bytes.push(b);
      this.node.push(null);
      this.index.push(0);
    }
  }
  /** a character of a text node, at index i of its value */
  char(s, node, i) {
    for (const b of new TextEncoder().encode(s)) {
      this.bytes.push(b);
      this.node.push(node);
      this.index.push(i);
    }
  }
  text(node) {
    const v = node.value;
    const protect = this.escapes(node);
    for (let i = 0; i < v.length; ) {
      const cp = v.codePointAt(i);
      const s = String.fromCodePoint(cp);
      const p = protect.get(i);
      if (p !== undefined) this.raw(p);
      else this.char(s, node, i);
      i += s.length;
    }
  }
  /**
   * The characters of the node's value that come from an escape or an
   * entity, with their source form: Map(index -> source). Empty when the
   * node has no position in this source or the source does not match.
   */
  escapes(node) {
    const out = new Map();
    const start = node.position?.start?.offset;
    const end = node.position?.end?.offset;
    if (this.source === undefined || start === undefined || end === undefined) return out;
    const src = this.source.slice(start, end);
    const v = node.value;
    if (src === v) return out;
    let i = 0;
    let k = 0;
    while (i < v.length && k < src.length) {
      if (src.charCodeAt(k) === BSL && k + 1 < src.length && src[k + 1] === v[i] && isPunctByte(src.charCodeAt(k + 1))) {
        out.set(i, src.slice(k, k + 2));
        i++;
        k += 2;
        continue;
      }
      if (src.charCodeAt(k) === AMP) {
        const m = /^&(#[0-9]{1,7}|#[xX][0-9a-fA-F]{1,6}|[A-Za-z][A-Za-z0-9]{0,31});/.exec(src.slice(k));
        if (m && src[k] !== v[i]) {
          // the entity's characters: the decoded text, one or two code points
          const cp = v.codePointAt(i);
          out.set(i, m[0]);
          i += String.fromCodePoint(cp).length;
          k += m[0].length;
          continue;
        }
      }
      if (src[k] === v[i]) {
        i++;
        k++;
        continue;
      }
      // a line break inside the text: the source has the next line's indent
      if (v[i] === "\n" && src[k] === "\n") {
        i++;
        k++;
        while (k < src.length && (src[k] === " " || src[k] === "\t" || src[k] === ">")) k++;
        continue;
      }
      if (v[i] === "\n" && /[ \t]/.test(src[k])) {
        k++;
        continue;
      }
      return new Map();
    }
    return i === v.length ? out : new Map();
  }
}

/** The stand-ins of the inline nodes */
function inline(line, node) {
  switch (node.type) {
    case "text":
      line.text(node);
      return;
    case "inlineCode":
      line.raw("`" + node.value + "`");
      return;
    case "html":
    case "mdxTextExpression":
      line.raw(node.type === "html" ? node.value : `{${node.value}}`);
      return;
    case "break":
      line.raw("\n");
      return;
    case "emphasis":
    case "strong":
    case "delete": {
      const m = node.type === "emphasis" ? "*" : node.type === "strong" ? "**" : "~~";
      line.raw(m);
      node.children.forEach((c) => inline(line, c));
      line.raw(m);
      return;
    }
    case "link":
      line.raw("[");
      node.children.forEach((c) => inline(line, c));
      line.raw(`](${node.url})`);
      return;
    case "linkReference":
      line.raw("[");
      node.children.forEach((c) => inline(line, c));
      line.raw("]");
      return;
    case "image":
      line.raw(`![${node.alt ?? ""}](${node.url})`);
      return;
    case "imageReference":
      line.raw(`![${node.alt ?? ""}]`);
      return;
    case "textDirective":
      // remark-directive's ":name" (Starlight writes it back as text when no
      // plugin takes it, as in ":x")
      line.raw(`:${node.name}`);
      if (node.children?.length) {
        line.raw("[");
        node.children.forEach((c) => inline(line, c));
        line.raw("]");
      }
      return;
    case "mdxJsxTextElement":
      line.raw(`<${node.name ?? ""}>`);
      node.children.forEach((c) => inline(line, c));
      line.raw(`</${node.name ?? ""}>`);
      return;
    default:
      if (node.children) node.children.forEach((c) => inline(line, c));
      else if (typeof node.value === "string") line.raw(node.value);
  }
}

/** Goldmark's scan over the line; returns the changes per text node */
function scan(line) {
  const bytes = Uint8Array.from(line.bytes);
  const counter = { single: 0, double: 0 };
  const changes = new Map();
  for (let pos = 0; pos < bytes.length; ) {
    const node = line.node[pos];
    if (node === null || !TRIGGER.has(bytes[pos])) {
      pos++;
      continue;
    }
    let end = bytes.indexOf(NL, pos);
    end = end < 0 ? bytes.length : end + 1;
    const before = pos === 0 ? "\n" : toRune(bytes, pos - 1);
    const r = parse(bytes.subarray(pos, end), before, counter);
    if (!r) {
      pos++;
      continue;
    }
    const [len, text] = r;
    // the replaced bytes are ASCII characters of the same text node
    for (let k = 1; k < len; k++)
      if (line.node[pos + k] !== node) throw new Error("remark-typographer: a substitution across nodes");
    if (!changes.has(node)) changes.set(node, []);
    changes.get(node).push([line.index[pos], len, text]);
    pos += len;
  }
  return changes;
}

function apply(changes) {
  for (const [node, list] of changes) {
    let v = node.value;
    for (const [i, len, text] of list.reverse()) v = v.slice(0, i) + text + v.slice(i + len);
    node.value = v;
  }
}

/** The typography of a block's inline nodes */
function block(node, source) {
  const line = new Line(source);
  node.children.forEach((c) => inline(line, c));
  apply(scan(line));
}

/** The typography of a plain string (the card's subtitle, as markdownify) */
export function typographString(s) {
  const node = { type: "text", value: s };
  const line = new Line(undefined);
  // the characters of s from..to, which the scan may change
  const chars = (from, to) => {
    for (let i = from; i < to; ) {
      const c = String.fromCodePoint(s.codePointAt(i));
      line.char(c, node, i);
      i += c.length;
    }
  };
  // code spans keep their text
  let last = 0;
  for (const m of s.matchAll(/`[^`]*`/g)) {
    chars(last, m.index);
    line.raw(m[0]);
    last = m.index + m[0].length;
  }
  chars(last, s.length);
  apply(scan(line));
  return node.value;
}

export function remarkTypographer() {
  return (tree, file) => {
    const source = typeof file?.value === "string" ? file.value : undefined;
    // a folder of the filetree: an item with a list, or a name ending in "/"
    const folder = (item) =>
      item.children.some((c) => c.type === "list") || /^\S*\/(\s|$)/.test(item.children[0]?.children?.[0]?.value ?? "");
    const visit = (node, tree = false) => {
      if (node.type === "mdxJsxFlowElement" && node.name === "FileTree") tree = true;
      if (tree && node.type === "listItem" && folder(node)) {
        node.children.filter((c) => c.type === "list").forEach((c) => visit(c, tree));
        return;
      }
      if (node.type === "mdxJsxFlowElement" && node.name === "LinkCard") {
        for (const a of node.attributes ?? [])
          if (a.type === "mdxJsxAttribute" && a.name === "description" && typeof a.value === "string") a.value = typographString(a.value);
      }
      if (node.type === "paragraph" || node.type === "heading" || node.type === "tableCell") {
        block(node, source);
        return;
      }
      if (node.type === "code" || node.type === "html") return;
      node.children?.forEach((c) => visit(c, tree));
    };
    visit(tree);
  };
}
