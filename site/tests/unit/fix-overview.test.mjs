// Unit tests of src/lib/fix-overview.mjs, run by `npm test` (node --test;
// the ctest site_unit).
import assert from "node:assert/strict";
import { test } from "node:test";
import { plainify } from "../../src/lib/fix-overview.mjs";

test("plainify removes HTML tags", () => {
  assert.equal(plainify("a <b>bold</b> <a href=\"x\">link</a>"), "a bold link");
  assert.equal(plainify("a line<br/>break"), "a linebreak");
});

test("plainify keeps < and > that are no tag", () => {
  assert.equal(plainify("the same as `<=`, not \"not equal\""), "the same as `<=`, not \"not equal\"");
  assert.equal(plainify("`<` and `>` compare"), "`<` and `>` compare");
  assert.equal(plainify("a < b > c"), "a < b > c");
});

test("plainify leaves no tag that removing another one joins", () => {
  assert.equal(plainify("<scr<b>ipt>x"), "x");
  assert.equal(plainify("<<b>i>x</<b>i>"), "x");
});

test("plainify of nothing is empty", () => {
  assert.equal(plainify(undefined), "");
  assert.equal(plainify(null), "");
});
