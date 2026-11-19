; rianimlib-delta-plane-offsets (#1120) - RINextAnimFrame takes the 16
; plane offsets at the start of a DLTA (the op lists, and for ANIM7 the
; data lists) without comparing them with the chunk's length.  32 x 16,
; 1 plane: frame 2's ANIM5 op list offset and frame 3's ANIM7 data list
; offset point behind their DLTA, at the ops of frame 4: frame 2 plays
; frame 4's ops (row 3 gets $AA), frame 3 writes those ops as its long
; (row 0 gets $02030001).
; Correct: frames 2 and 3 are refused (RINextAnimFrame returns 0, rows 0
; and 3 stay 0); frame 4 plays (row 3 gets $AA).
; @fixed #1120
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-delta-plane-offsets.anim
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
NPrint "frame 2: ",r," row 3 ",Hex$(Peek.b(p+12) AND $FF)
r=RINextAnimFrame(0)
NPrint "frame 3: ",r," row 0 ",Hex$(Peek.l(p))
r=RINextAnimFrame(0)
NPrint "frame 4: ",r," row 3 ",Hex$(Peek.b(p+12) AND $FF)
FreeMem_ a,4096
End
