; animlib-zero-dimensions-load (#402) - InitAnim, its second form and
; NextFrame count rows, planes and columns with SUBQ #1 ... DBF: an ANIM
; whose BMHD has width, height or depth 0 runs those loops 65536 times
; (with depth 0 over 65536 plane pointers, with width 0 over 65536
; columns per plane in NextFrame).  The depth and width cases cannot be
; run without writing far outside the bitmap, so this test watches what
; LoadAnim keeps of such files (the height case: animlib-zero-dimensions).
; Here: LoadAnim of an ANIM with depth 0 and of one with width 0 (both
; generated, a BODY of one row and two DLTA chunks), the BODY length
; (+14), the number of frames and whether there is a delta (+18).
; Correct: a picture with a dimension 0 has no data to decode: LoadAnim
; keeps no BODY and no deltas, so InitAnim and NextFrame do nothing.
; The original keeps both.
; @fixed #402
; @configs clib
; @pass file=depth0.anim|@TESTDATA@/animlib-zero-dimensions-load-depth.anim
; @pass file=width0.anim|@TESTDATA@/animlib-zero-dimensions-load-width.anim
DEFTYPE .l
LoadAnim 0,"CORPUS:depth0.anim"
a=Addr Anim(0)
NPrint "depth 0: body ",Peek.l(a+14)," frames ",Frames(0)," deltas ",Peek.l(a+18)<>0
LoadAnim 1,"CORPUS:width0.anim"
a=Addr Anim(1)
NPrint "width 0: body ",Peek.l(a+14)," frames ",Frames(1)," deltas ",Peek.l(a+18)<>0
End
