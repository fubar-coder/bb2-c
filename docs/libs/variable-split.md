# Variables with several meanings in the library sources

Stage 2 of the readable names. Stage 1 (2026-10-31) was a pure rename:
`libs/acidlibs`, `libs/userlibs/rwe` (`libs/deflibs` needed none), with
every output byte-identical. Where one variable of the translation, often
one register of the original, holds several unrelated values, stage 1
gave it a neutral name. Stage 2 splits it: one variable per value. A
split changes the generated code (register allocation), so it is not
byte-identical. It is checked with the tests (`ctest --preset all`, the
library's emulator tests, `ctest --preset bug-orig`) and named in the
commit. If a split fixes an original bug, that is an intended deviation
with its issue and a test (CLAUDE.md).

Line numbers are as of the end of stage 1 (`77847c9`). In the lists,
"x, then y" means the variable first holds x and later y.

State: the acidlibs part is done (2026-10-31, one commit per library,
"acidlibs: split the multi-use variables of X.c"; the new names are in
those commits). Most of the libraries came out byte-identical (GCC
allocated the same registers); the code changed in animlib, asllib,
blitzcoplib, blitzkeyslib, crunchlib, datelib, displaylib,
linklistlib, lmullib, printlib and vallib, checked with libs_emulator,
the corpus and the libraries' bug-orig tests. No split changed a
behaviour; where the original depends on a reused register (asllib's
empty font name returns 0, trackerlib's song end after a break
BUG(orig) (#292), the upper words kept by word loads), the split keeps
it and says so in a comment. The userlibs/rwe part is done as well
(2026-10-31, "rwe: split the multi-use variables of X.c", and the rwe
statics); the code changed in rishapeslib, ritrackerlib, richunkylib,
rievallib, ristcdecrunchlib and ristonecrackerlib. Left as they are
because the original depends on them: rievallib's `aux` in ev_loop
(read after a register in `{}`) and `acc` in ctp_loop, the riseriallib
openserial/writeser/readser chain (BUG(orig) #797, #801), rifxlib
asmderez's value after yloop (BUG(orig) #672), riwbspritelib's
`wd.temppointer` (compared by libs_emulator).

## Kept in stage 1, renamed in stage 2

- **File-scope statics:** their name is in the symbol table of the
  intermediate `.c.obj`, so renaming one is not byte-identical.
  - rundebuglib.c:62-64 `t1`, `t2`, `t3` (IntuiText: message, two
    "OKAY!" gadgets)
  - shapeslib.c:691-692 `bmode`, `offs`
  - 2dlib.c:255 `tempadd`
  - fileiolib.c:277 and inputoutputlib.c:128 `temp`
  - iffmakelib.c:120-122 `tempcols`, `temppal`, `tempbm`
  - rianimlib `mytemp`, `animwidthtemp`
  - rimisclib `tempstorage`
  - ciatrklib `g`, `c`, `p`
  - neilsreqtoolslib `a3_`
  - ridisasmlib `a0`, `a2`, `a5`, `l1`, `l2`
- **Register-level translations:** the static `d0`-`d7` and `a0`-`a6`
  are the emulated 68000 registers, and each holds hundreds of values.
  These need a rewrite, not a split, so they are their own item:
  2dlib.c:195-196 (with the saved copies `sa3`, `sa4`, `s0`, ...),
  medlib.c and polylib.c. All three are rewritten (2026-10-31: plain C;
  the edge lists `l0`/`l1` of the BUG(orig) #395 text are `ledges`/
  `redges`; medlib keeps the register values its results depend on
  explicitly, #203/#205).
- **Register-pinned bindings** (`register T d0 __asm("d0")`), which are
  the register itself: these stay.
  - debuglib callhandler `ra0`, `ra1`, `rd0`
  - errtraplib `d0`, `d1`
  - runerrlib `spfix`
- **Original labels used as cross-references** stay: crunchlib's
  im26/im28/..., debuglib's image labels `d0_`..`a7_`, animlib's
  ALIBJSR_Z bindings `_d0`/`_a0`.

## acidlibs

- **displaylib.c**
  - :541 initcoplist:
    - `val` (d0): the sprite-mode value/offset, the AGA flags, the list
      size, the list address, then the resolution.
    - `aux` (d7): the AGA bank-move count, then COPNOP.
  - :537 initcoplist `type` (d3): the type, then the bitplane count,
    then the bitplane move count.
  - :606 `modeoff` (d1w): the mode table offset, then the resolution
    index.
  - :760 _initcoplist2_c `ypos` (d1): arrives as the type, then becomes
    the ypos.
  - :852 displaybmap2 `setting` (d5): the fetch width, then the HAM8
    flag.
  - :1073 _displaysprite_c: every register is reused. The parameters
    carry the names of their later values (a comment explains this).
    - `vpos` (d2): x, then y/vstart/vstop.
    - `chan` (d3): y, then the channel.
    - `handle` (d4): the channel, then the x handle, then the data skip.
    - `height` (d5): the width check, then the y handle, then the
      height.
    - `lines` (d0): the visible lines, then the data address.
    - `hpos` (d1): the x position, then the data step.
    - `posword`/`ctlword` (d6/d7): the sprite count, SPRI, resshift
      and SPIF, then the SPRxPOS/SPRxCTL words.
  - :1705 scroll:
    - `pos1` (d0): the resolution bits, then x.
    - `mod2` (d4): a mask, then the modulo.
    - `rot` (d6): the flups index, then the rotate count.
  - :1790 _customchunkysize_c `size` (d0): the width, then the size.
  - :1802 customchunky `width` (d3): the width, then the bytes per line.
- **blitzoslib.c**
  - :517 drawbox `darklight` (d0): the 0/-1 disabled flag, then the
    c0:c1 pen pair.
  - :608 `limit` (d0w): half the visible lines, then the highest first
    entry.
  - :660 `ptr`/`len` (d0l/d1l): hold tlen/text until the EXG swaps them.
  - :706 `col`: the text length, then the column.
- **gadgetslib.c**
  - :656 `boxw`/`boxh`: the border excess, then the width/height.
  - :950 `result`: the GadgetText pointer, then the part number.
- **crunchlib.c**
  - :169 `infores`: the info routine pointer, then its inverted result.
  - :276 `len`: the match length, then 5, then the length-2 table index.
  - :418 implode:
    - `num`: the table index, then the length code bits, then the
      crunched length.
    - `val`: the table number, the window, bit values, then the gain.
  - :624 `extra`: the literal count base, then the offset bits value.
  - :930 `sym`: the Huffman symbol, then the distance bit count, then
    the distance.
- **blitzcoplib.c**
  - :357 `size`: the bytes to insert, the new list size, then the long
    count of the copy.
  - :362 `wline`: the wait line, then the "insert the line-256 wait"
    flag.
  - :487 `val`: DDFSTRT, BPLCON0, the DIW values, the modulo base, the
    plane count, then the end-wait line.
  - :843/855/867 `x`: x, then the BPLCON1 scroll value.
- **fileiolib.c**
  - :633 transdata `result`: the file number, then the results of
    myread/mywrite.
- **linklistlib.c**
  - :310 `item`: the killed item, then its predecessor.
  - :330/352/371/391/410 `result`: the link pointer, then -1.
- **animlib.c**
  - :200 `nodeval`: the next pointer, then the length.
  - :235 doanimload `val`: the BODY length, the BODY memory, the DLTA
    length, the length+8, then the node pointer.
  - :~398 donextframe_c `val`: the delta offset, then the next node.
- **lmullib.c**
  - :40/69 `acc` (d0): |a|, then the product.
  - :40 `cross_hilo`/`cross_lohi`: copies of |a|, then the partial
    products.
- **blitzkeyslib.c**
  - :145 `posqual` (d1): the buffer position, then the qualifier.
- **asllib.c**
  - :199 `result` (d7): the request result, then the font name length.
- **datelib.c**
  - :201 datestr_c:
    - `year`: the 4-year cycles, then the year, then the DIVU result.
    - `month`: the days, then the month.
  - :274 calcdays_c:
    - `num`: the field, then year/4.
    - `digit`: the character, then the leap days.
    - `day`/`month`: swapped by EXG; `month` is then doubled into a
      table offset.
- **ilbmifflib.c**
  - :300 `rowsize`: the bytes per row, then words-1.
- **iffmakelib.c**
  - :196 `count`: the colour count, then the DBF counter.
  - :218 `byte`: the first byte of a run, then the compared byte.
- **printlib.c**
  - :277 `count` (d5): the position counter, then the overflow amount,
    then the loop counter for the lead/trail places.
  - :277 `lead` (d3): the leading digit count, then the leading
    spaces/zeros still to write.
  - :623 `edig` (d2): the absolute exponent for the "E+nn" text, then
    the tens digit of the exponent.
  - :617 `num` (d0): the FFP number, then the mantissa being reduced.
- **rawkeylib.c**
  - :109 `qual` (d1): the qualifier in, then the converted character
    out.
  - :190 `result` (d0): the OpenDevice error, then key codes/converted
    characters.
- **scrolllib.c**
  - :382 and :407 `bits` (d4): the shift difference/BLTCON shift, then
    an edge mask.
  - :382 and :407 `pos` (d3): the dest x bits (dodesc), then the cookie
    offset.
  - :483 `size` (d2): the byte width, then BLTSIZE.
- **spriteslib.c**
  - :293 `vpos` (d2): the start line, then the stop line.
- **trackerlib.c**
  - :285 `val` (d0, mt_vib): the sine index, then the depth, then the
    period.
  - :446 `mod` (a0, mt_music): the module base, then the patterns
    pointer, then the song length/restart bytes.
- **vallib.c**
  - :561 `num` (d7, notneg): the integer part, then the fraction, then
    the fraction as float.
- **stringfunclib.c**
  - :740 `other` (a1, Replace$): the find-string pointer, then the
    replacement-string pointer.
- **shapeslib.c**
  - :428 `size` (d5, initshape): BLTSIZE, then the bytes per row plus
    the extra word.
  - :696-698 doshload_:
    - `res` (d0): the bltsize word at the top, then the result of the
      chained IFF call, passed back as d0.
    - `dst` (a2): the shape data write pointer, then the next frame's
      shape object.
    - `pstep` (d7): the plane size, then the step from the row end to
      the next plane.
- **shapetrixlib.c**
  - :215 `val` (d0, xflip): the pad bits, then the BLTCON0 shift, then
    BLTSIZE-1.
- **The incoming d0 kept only for its upper word:**
  - palettelib.c:410
  - menuslib `checked`/`domenuchecked*` `prev`
  - qblitlib/vallib `codeinit`/`dodispheight`/`dontsc` `prev`

## userlibs/rwe

- **popupwinlib.c**
  - :470 `result` (dopop, d0): an IDCMP event class or a sub-level
    result, then the return value.
- **blitzbomberslibtwo.c**
  - :73 `img` (a0): the table entry address, then the image pointer.
- **rimapshapeslib.c**
  - :323 `xbase` (blitgroup_c, d2): y on entry, then x.
- **rimapeditorlib.c**
  - :384 `shapenum` (candraw_c, d0): the shape number, then the
    rub-through word - 1, then the result.
- **rishapeslib.c**
  - :496 `work` (shapesizes, d0): the bytes per row, the words per row,
    the height, then 2*height.
  - :845 (and :856) `maxshapes` (d4): the maximum count, then the byte
    offset of the last shape. It is also passed as the second shape
    number.
  - :992 (and :1030) `shapes` (d1): the second shape number, then that
    number >> 5, then the count.
- **ritrackerlib.c**
  - :497 `arp` (d0): the counter % 3, then the note step.
  - :574 `work`: the period-table offset, then the current period.
  - :605 `work`: the porta speed, then the table offset.
  - :700 `work`: the effect argument, then the tremolo position.
  - :855 `value`: the filter bit (E0x) or the funk speed (EFx).
  - :1058 `work` (d2): the sample number, then its start-table offset,
    then the loop start.
- **ciatrklib.c**
  - :513 `timerval` (d7): the PAL/NTSC clock, then clock/125.
  - :1427 `val` (loadtrkmodule_c, d0): the slot number, then the file
    size, then the Read result.
  - :1539 `val` (getmodulename_c, d0): the slot number, then the name
    length.
  - :702 `bits`: the active channel mask and its INTREQ bits, then the
    same for the channels to start.
- **rizonejoylib.c**
  - :318 `zoneval` (d0): the zone number, then its byte offset.
- **optidec.h**
  - :38 `bits` (struct stc, d1): the getbits count, getbits' result, and
    a literal run's shift count.
- **ripacklib.c**
  - :358 `value`: the extra length bits, then the string offset.
  - :465 `count` (d7): the BMHD byte width, then the BODY search
    countdown.
- **riarexxlib.c**
  - :721 `result`: the passed-through d0, or the first message.
  - :836 `value` (d0): the argument number, then the argument string.
- **rilesdebuglib.c**
  - :841 `code` (d0): the token number, then draw_char's glyph.
- **rievallib.c** (evaluator state at register level)
  - :162 `acc` (d0): a character, operator-stack entries, a string
    pointer, the number being built, a register index and value, an
    indirection value, an operator result.
  - :162 `aux` (d1): the previous character, the popped stack top, a
    buffer pointer, the caller's d1, a digit, the multiplier and the
    divisor.
  - :483 `rhs`: the right operand, then the exponent loop count for `^`.
- **rifxlib.c**
  - :867/875 `value` (asmderez_c, d0): the caller's d0, a DBRA
    leftover, the source word, then the result.
  - :875 `pos` (asmderez_c, a2): the source line, then the next
    destination line.
  - :875 `bits` (asmderez_c, d2): a mask, a rotated copy, then the
    destination word.
  - :1062 `pos` (asmzoom_c, a2): the source pointer, then the next
    destination line.
- **ritooltypeslib.c**
  - :675 `count`: the plane size in bytes, then the DBRA word count.
- **richunkylib.c**
  - :245 `px0`/`px1`/`mix0`/`mix1`/`bits` (C2P d0-d4): the input longs,
    the nibble halves, the merged nibbles, the masks.
- **rilvcallhooklib.c**
  - :281 `value` (d0): the indent offset, the indent, then the front
    pen.
- **riseriallib.c**
  - :435 `value` (c_sendserialstring, d0): the string argument, then
    openserial's result, then writeser's result.
  - :451 `value` (c_readserialstring, d0): the caller's d0, then 0 from
    openserial, then each byte read.
- **riwbspritelib.c**
  - :121 `wd.temppointer`: the new sprite data in newsprite, then the
    table entry address in freesprite_c. It is a field of the original's
    data block.
