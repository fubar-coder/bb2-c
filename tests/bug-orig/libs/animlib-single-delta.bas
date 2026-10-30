; animlib-single-delta (#401) - after applying a delta NextFrame goes to
; the next node; at the end of the list it wraps to the node AFTER the
; first one.  With a single DLTA that link is 0: the current delta (+26)
; becomes 0 and the next NextFrame reads its plane offsets from address 8.
; Here: an ANIM (generated) with one DLTA that changes nothing; InitAnim,
; one NextFrame, then the program reads the current delta node (+26) -
; the second NextFrame is not done.
; Correct: the current delta is still a delta node (not 0).
; @configs clib
; @pass file=onedelta.anim|@TESTDATA@/animlib-single-delta.anim
DEFTYPE .l
BitMap 0,16,1,1
LoadAnim 0,"CORPUS:onedelta.anim"
a=Addr Anim(0)
NPrint "frames ",Frames(0)," deltas ",Peek.l(a+18)<>0
InitAnim 0
NPrint "init current ",Peek.l(a+26)=Peek.l(a+18)
NextFrame 0
NPrint "row ",Hex$(Peek.w(Peek.l(Addr BitMap(0)+8)) AND $FFFF)
NPrint "current is a node ",Peek.l(a+26)<>0
End
