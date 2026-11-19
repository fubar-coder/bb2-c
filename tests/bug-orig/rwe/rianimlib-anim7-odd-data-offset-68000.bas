; rianimlib-anim7-odd-data-offset-68000 (#1121) - the ANIM7 decoder of
; RINextAnimFrame reads the data list of a plane (longs, or words in the
; short form) at the offset the DLTA gives; an odd offset makes every
; read odd: on a 68000 that is an address error (the session ends in a
; timeout).  The machine is the A600 (68000).  32 x 16, 1 plane, ANIM7
; long data: frame 2's data list of plane 0 is at the odd offset 67,
; frame 3's at 68 (row 0 gets $11223344).
; Correct: frame 2 is refused (RINextAnimFrame returns 0, row 0 stays 0);
; frame 3 plays (row 0 gets $11223344).
; @fixed #1121
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @machine a600
; @pass timeout=30
; @pass file=anim|@TESTDATA@/rianimlib-anim7-odd-data-offset-68000.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,32,16,1
r=RIAnimInit(a,0,0)
p=Peek.l(Addr BitMap(0)+8)
NPrint "before"
r=RINextAnimFrame(0)
NPrint "frame 2: ",r," row 0 ",Hex$(Peek.l(p))
r=RINextAnimFrame(0)
NPrint "frame 3: ",r," row 0 ",Hex$(Peek.l(p))
FreeMem_ a,4096
End
