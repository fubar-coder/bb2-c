; rianimlib-init-xy-offset (#514) - RIAnimInit's x,y form turns the
; position into a byte offset into the planes (BytesPerRow * y with
; MULU.w, + x >> 3) without looking at the bitmap: a negative y or one of
; 65536 and above gives an offset of up to 65535 rows, and UnpackIFF and
; every RINextAnimFrame write there.  The original has no error check
; area.  Here with runtime error checking and x = 32 on a bitmap 32
; pixels wide (the animation is one row high, the offset of 4 bytes stays
; inside the plane in the original).
; Correct: the error check refuses the position: "Animation position
; outside the bitmap".
; @fixed #514
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass runtime_errors=on
; @pass file=anim|@TESTDATA@/rianimlib-init-xy-offset.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,32,16,1
NPrint "before"
r=RIAnimInit(a,0,0,32,0)
NPrint "after ",r
FreeMem_ a,4096
End
