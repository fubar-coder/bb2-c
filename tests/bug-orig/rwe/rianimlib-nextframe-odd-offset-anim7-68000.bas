; rianimlib-nextframe-odd-offset-anim7-68000 (#1122) - RIAnimInit's
; xy_offset form adds the byte offset to the plane pointers; for an ANIM7
; frame an odd offset makes the decoder's word or long accesses to the
; planes odd: on a 68000 that RINextAnimFrame is an address error (the
; session ends in a timeout).  The machine is the A600 (68000).  Here
; with runtime error checking on a bitmap 64 pixels wide, an ANIM7 of
; 32 x 16, 1 plane, long data: first xy_offset 2 (even: row 0 gets
; $11223344 at byte 2), then xy_offset 1.
; Correct: the even offset plays; at the odd one RINextAnimFrame's error
; check refuses the odd plane address: "Animation position on an odd
; byte for ANIM7".
; @fixed #1122
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @machine a600
; @pass runtime_errors=on
; @pass timeout=30
; @pass file=anim|@TESTDATA@/rianimlib-nextframe-odd-offset-anim7-68000.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,64,16,1
p=Peek.l(Addr BitMap(0)+8)
r=RIAnimInit(a,0,0,2)
NPrint "init 2: ",r
r=RINextAnimFrame(0)
NPrint "frame 2: ",r," ",Hex$(Peek.l(p+2))
r=RIAnimInit(a,0,0,1)
NPrint "init 1: ",r
r=RINextAnimFrame(0)
NPrint "frame 2: ",r
FreeMem_ a,4096
End
