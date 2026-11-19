; rianimlib-nextframe-odd-x-anim7-68000 (#1122) - RIAnimInit's x,y form
; adds the byte offset x >> 3 to the plane pointers; for an ANIM7 frame
; an odd byte offset makes the decoder's word or long accesses to the
; planes odd: on a 68000 that RINextAnimFrame is an address error (the
; session ends in a timeout).  The machine is the A600 (68000).  Here
; with runtime error checking, x = 8 (byte offset 1) on a bitmap 64
; pixels wide; the ANIM (32 x 16, 1 plane) has an ANIM5 frame 2 (bytes:
; plays at any x, byte 1 of row 0 gets $AA) and an ANIM7 frame 3 (long
; data).
; Correct: frame 2 plays; at frame 3 RINextAnimFrame's error check
; refuses the odd plane address: "Animation position on an odd byte for
; ANIM7".
; @fixed #1122
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @machine a600
; @pass runtime_errors=on
; @pass timeout=30
; @pass file=anim|@TESTDATA@/rianimlib-nextframe-odd-x-anim7-68000.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,64,16,1
r=RIAnimInit(a,0,0,8,0)
NPrint "init ",r
p=Peek.l(Addr BitMap(0)+8)
r=RINextAnimFrame(0)
NPrint "frame 2: ",r," row 0 byte 1 ",Hex$(Peek.b(p+1) AND $FF)
r=RINextAnimFrame(0)
NPrint "frame 3: ",r
FreeMem_ a,4096
End
