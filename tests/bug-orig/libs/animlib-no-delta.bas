; animlib-no-delta (#401) - NextFrame applies the current delta node
; without a check: an ANIM with an ANHD but no DLTA has no delta node
; (+18 = 0), InitAnim makes the current delta (+26) 0, and NextFrame
; reads its delta node at address 0: plane offsets from address 8 (the
; exception vectors), ops from wherever they point, written into the
; bitmap.
; Here: an ANIM (generated) of one row $AABB with an ANHD and no DLTA;
; InitAnim, NextFrame, then the row.
; Correct: without a delta NextFrame does nothing: the row stays $AABB.
; @fixed #401
; @configs clib
; @pass file=nodelta.anim|@TESTDATA@/animlib-no-delta.anim
DEFTYPE .l
BitMap 0,16,1,1
LoadAnim 0,"CORPUS:nodelta.anim"
a=Addr Anim(0)
NPrint "frames ",Frames(0)," deltas ",Peek.l(a+18)<>0
InitAnim 0
NPrint "init row ",Hex$(Peek.w(Peek.l(Addr BitMap(0)+8)) AND $FFFF)
NextFrame 0
NPrint "row ",Hex$(Peek.w(Peek.l(Addr BitMap(0)+8)) AND $FFFF)
End
