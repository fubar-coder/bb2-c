; rianimlib-delta-depth (#509) - the delta plane loop runs up to the depth
; of the BMHD (a byte counter), not checked against the 8 plane pointers
; of a BitMap: a depth of 9 takes the 9th plane pointer from behind them
; (offset 40 of the bitmap object) and the 9th offset of the DLTA.
; (Depth 0 runs 256 planes; RIPackLib's UnpackIFF fails on it first,
; #777.)  The 9th "plane pointer" is set to a block of the program's own
; for the call (and restored); UnpackIFF refuses the depth of 9 for the
; first frame.
; Correct: a depth above 8 is refused, the block stays 0.
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-delta-depth.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,16,16,1
o=Addr BitMap(0)
b=AllocMem_(64,$10001)
w=Peek.l(o+40)
Poke.l o+40,b
r=RIAnimInit(a,0,0)
r=RINextAnimFrame(0)
v=Peek.b(b) AND 255
Poke.l o+40,w
FreeMem_ b,64
FreeMem_ a,4096
If v Then NPrint "9th plane written" Else NPrint "no 9th plane"
End
