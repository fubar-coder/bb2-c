# Token table of Blitz Basic 2.1

As of 26.10.2026. Source: Blitz Basic CD (`archive/cd/ultimate-blitz-basic-2.1/`, paths in lower case), libraries
`blitz2/deflibs` (standard), `extras/bigdeflibs/deflibs` (large variant) and
`blitz2/acidlibs`. The files have **the same BigLib format** as the
BlitzLibs of AmiBlitz3 (`tools/blitztokens` lists the tokens of both).

The built-in commands (lib 0/1, words `$80xx`) are not in the
libraries but in the compiler `blitz2/blitz2` (see `formats.md`).

## Files

| File (bb2 only, not taken over) | Contents |
|---|---|
| `tokens/deflibs.txt` | `blitz2/deflibs`, 992 tokens, libs 2 and 200–255 (OS calls `…_`) |
| `tokens/bigdeflibs.txt` | `extras/bigdeflibs/deflibs`, 1408 tokens: like `deflibs` without lib 200, plus libs 11–111 (third-party libs: lists, requesters, modules, `Request`, file system …) |
| `tokens/acidlibs.txt` | `blitz2/acidlibs`, 619 tokens, libs 74–194 (Acid commands: strings lib 185, `Edit$`/`Inkey$` lib 186 …) |
| `tokens/ab3.txt` | AmiBlitz3 for comparison (`ab3c -dump token` with `AmiBlitz3-Original/BlitzLibs`), 4653 lines |
| `tokens/vergleich.txt` | every BB2.1 token with its status relative to AB3 |
| `tokens/corpus_unbekannt.txt` | the words of the Aminet corpus unknown to AB3, with BB2.1 names |

Generated with `tools/compare_tokens.py` and `tools/resolve_unknown.py` (in bb2, not taken over):

```
blitztokens -o tokens/acidlibs.txt cd/blitz2/blitz2 cd/blitz2/acidlibs
python3 -I tools/compare_tokens.py tokens/ab3.txt tokens/deflibs.txt \
    tokens/bigdeflibs.txt tokens/acidlibs.txt > tokens/vergleich.txt
python3 -I tools/resolve_unknown.py \
    ../ab3c/tests/external/aminet/bb2-corpus-files.csv \
    tokens/deflibs.txt tokens/bigdeflibs.txt tokens/acidlibs.txt
```

## Comparison with AmiBlitz3 (lib/index → name)

| Status | Count | Meaning |
|---|---|---|
| same | 1706 | same name under the same number |
| other name | 223 | number exists in AB3, but with a **different** command |
| missing | 102 | number does not exist in AB3 |

The 223 cases with a different name are mostly **shifted indices**: AB3 has
inserted a command into a lib or removed one, and all following ones move up.
Examples:

| Word | BB2.1 | AB3 |
|---|---|---|
| `$A589` (75/9) | `Min` | `CopyByte` |
| `$A58D` (75/13) | `Max` | `BLoad` |
| `$9C0C` (56/12) | `ModuleToChip` | `GetModuleName$` |
| `$B581` (107/1) | `Null` | `CreateMsgPort` |
| `$BC0E` (120/14) | `True` | `Vallong` |
| `$F581` (235/1) | `GetIcon_` | `FreeFreeList_` |
| `$A781` (79/1) | `ReqFileRequest` | `Edit$` |

Consequence for the converter: a token word of a BB2.1 file that AB3 knows is
**no** proof that it means the same. A `.bb2` file can only be
translated with the table of the version it was saved with.
Which version that was is not stated in the file (header `BB22`, see
`formats.md`); the converter needs the version as an option or a
plausibility check (e.g. number of parameters).

## The old words from the Aminet corpus

`docs/bb2-converter/bb2-corpus.md` §5 lists words that AB3 does not know
(CSV: 57 distinct). With BB2.1, **37** of them are named
(`tokens/corpus_unbekannt.txt`):

| Words | Lib | BB2.1 | Files |
|---|---|---|---|
| `$DC81`–`$DC99` | 185 (acidlibs) | `Left$`, `Right$`, `Mid$`, `Hex$`, `Bin$`, `Chr$`, `Asc`, `String$`, `Instr`, `Replace$`, `Mki$` … `CaseSense` | up to 100 per word |
| `$DD01`–`$DD03` | 186 (acidlibs) | `Edit$`, `Inkey$`, `Edit` | 57/90/16 |
| `$B58D`, `$B58F`–`$B591` | 107 (bigdeflibs) | `Request`, `ScreenWidth`, `ScreenHeight`, `ActiveWindow` | 13/21/21/2 |
| `$A596`, `$A599` | 75 | `NextBank`, `FillMem` | 7/3 |
| `$B38C` | 103 | `ClickMouse` | 1 |
| `$C70D` | 142 | `BlitzFont` | 1 |
| `$FD28` | 250 | `BestModeIDA_` | 2 |

The mapping in `bb2-corpus.md` §5, which had been inferred from context
(lib 185 → today lib 6 same index, lib 186 → lib 79), agrees
with this. **Not** BB2.1 are 20 words: lib 5, 29, 39, 109/27–48, 123, 147
– they come from later or third-party libraries (AmiBlitz 2, third-party libs).

## Relation to `replacedat` and the broken resident macros

`replacedat` in AmiBlitz3 only converts FPU variants (finding of 26.10.,
change `macros` in ab3c); AB3 does **not** map old lib numbers. The
words in the five broken macros of `all.res` are exactly BB2.1 words:

| Word in `all.res` | BB2.1 |
|---|---|
| `$DC86` | `Chr$` (185/6) |
| `$DC87` | `Asc` (185/7) |
| `$B58D` | `Request` (107/13) |
| `$B593` | `ShowRequesters` (107/19) |

So the macros (`PSD_NAME_FRONTMOST`, `Toolbar_KeyButton`, `checklib`, `cout`,
`screen_`) were tokenised with a Blitz 2 table and never
converted. A converter for `.bb2` has to bring the BB2.1 table
itself; `replacedat` does not help with that. This corrects the assumption in
`bb2-corpus.md` §5 and requirement 4 in `docs/bb2-converter/README.md`.

## Open

* Whether files of the corpus contain known but **shifted** words
  (e.g. `$A589` as `Min`): for this, run the token count per file
  (`tests/external/aminet/tools/analyze.py`) against
  `vergleich.txt`.
* Older versions (2.0, 1.x) and AmiBlitz 2 have their own tables; the CD
  contains only 2.1 (the `bum*` disks possibly intermediate states).
