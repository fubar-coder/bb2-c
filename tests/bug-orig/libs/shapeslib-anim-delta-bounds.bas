; shapeslib-anim-delta-bounds (#1193) - LoadShape of an ANIM applies the ops of a
; DLTA column (skip rows, unique bytes, a run of one byte) without a check
; against the height of the frame: an op that goes past the last row
; writes into the next plane, and from the last plane on behind the
; shape's data.  A DLTA that ends early or is missing is read as $FF
; bytes (iffbyte's -1 at the end of the file), 255 ops of 127 bytes per
; column, far behind the data.
; Here (generated) an ANIM of 16 x 1 pixels with two planes (plane 0
; $0102, plane 1 $0304) and one delta frame whose plane 0 column 0 skips
; one row and sets the byte there: that is plane 1's first byte, inside
; the shape's own data (nothing else is overwritten).
; Correct: the delta stays inside its plane, plane 1 of frame 1 is $0304.
; @configs clib
; @pass file=anim.iff|@TESTDATA@/shapeslib-anim-delta-bounds.anim
DEFTYPE .l
LoadShape 0,"CORPUS:anim.iff"
d=Peek.l(Addr Shape(1)+14)
NPrint "plane 0 ",Right$(Hex$(Peek.w(d) AND $FFFF),4)
NPrint "plane 1 ",Right$(Hex$(Peek.w(d+2) AND $FFFF),4)
End
