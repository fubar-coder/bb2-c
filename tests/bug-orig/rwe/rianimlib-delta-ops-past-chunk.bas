; rianimlib-delta-ops-past-chunk (#1120, the part kept) - the ops of a
; DLTA's columns are not bounded by the chunk: a column whose op count
; claims more ops than the DLTA holds reads them behind it.  32 x 16, 1
; plane: frame 2's column 0 claims 2 ops, the DLTA ends behind the first
; (skip 3 rows); the second, behind the chunk, copies $AA to row 3.
; Kept on purpose (user decision "C2 findings" of 2026-11-19): a check
; per op would cost every delta op of RINextAnimFrame (D-007).
; Correct: no op behind the DLTA is played (row 3 stays 0).
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-delta-ops-past-chunk.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,32,16,1
r=RIAnimInit(a,0,0)
p=Peek.l(Addr BitMap(0)+8)
r=RINextAnimFrame(0)
NPrint "row 3 ",Hex$(Peek.b(p+12) AND $FF)
FreeMem_ a,4096
End
