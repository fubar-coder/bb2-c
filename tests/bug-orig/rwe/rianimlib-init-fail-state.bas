; rianimlib-init-fail-state (#512) - RIAnimInit stores the new ANIM's
; address and end before it checks for the FORM ILBM and its BMHD; when
; one is missing it returns 0 but keeps them, and the frame pointers of
; the animation initialised before.  A good RIAnimInit (two frames), then
; one of a FORM ANIM whose only frame has no BMHD (built by the program
; behind the file): RIAnimFrameCount counts the rejected ANIM's chunk,
; RINextAnimFrame plays the first animation's second frame.
; Correct: after the failed RIAnimInit there is no animation:
; RIAnimFrameCount and RINextAnimFrame return 0.
; @fixed #512
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-init-fail-state.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,16,16,1
r=RIAnimInit(a,0,0)
NPrint "good ",r
b=a+2048                        ; FORM ANIM { FORM ILBM { CMAP } }
Poke.l b,$464F524D : Poke.l b+4,24 : Poke.l b+8,$414E494D
Poke.l b+12,$464F524D : Poke.l b+16,12 : Poke.l b+20,$494C424D
Poke.l b+24,$434D4150 : Poke.l b+28,0
r=RIAnimInit(b,0,0)
NPrint "bad ",r
c.w=RIAnimFrameCount
NPrint "count ",c
r=RINextAnimFrame(0)
NPrint "next ",r
FreeMem_ a,4096
End
