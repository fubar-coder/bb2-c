// Markdown text outside its code blocks, for the converter of the Hugo
// pages (scripts/convert-hugo.mjs) and the site's checks
// (src/integrations/site-checks.mjs), which look only at the text that a
// reader sees as text.

/**
 * Splits a Markdown text into code blocks and the text between them
 * (fences of ``` or ~~~, at any indentation, closed by the same character at
 * least as often). Returns [{code, text}].
 */
export function segments(src) {
  const out = [];
  let cur = [];
  let fence = null;
  for (const line of src.split(/(?<=\n)/)) {
    const m = /^[ \t]*(`{3,}|~{3,})/.exec(line);
    if (!fence && m) {
      if (cur.length) out.push({ code: false, text: cur.join("") });
      cur = [line];
      fence = m[1];
    } else if (fence && m && m[1][0] === fence[0] && m[1].length >= fence.length && /^[ \t]*[`~]+[ \t]*\r?\n?$/.test(line)) {
      cur.push(line);
      out.push({ code: true, text: cur.join("") });
      cur = [];
      fence = null;
    } else cur.push(line);
  }
  if (cur.length) out.push({ code: !!fence, text: cur.join("") });
  return out;
}

/** fn applied to the text outside code blocks */
export const outsideCode = (src, fn) => segments(src).map((s) => (s.code ? s.text : fn(s.text))).join("");

/** The line number of offset i in src */
export const lineOf = (src, i) => src.slice(0, i).split("\n").length;
