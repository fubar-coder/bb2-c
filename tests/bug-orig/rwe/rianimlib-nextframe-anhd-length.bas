; rianimlib-nextframe-anhd-length (#1117) - RINextAnimFrame steps over
; the ANHD (and a CMAP) of a frame by its length without comparing the
; step with the frame's end.  The ANHD of frame 2 (16 x 16, 1 plane)
; claims a length that ends at the DLTA of frame 3, behind frame 2: the
; first RINextAnimFrame plays frame 3's delta (row 1 of column 0 gets
; $0F) instead of frame 2's (row 0 gets $F0).
; Correct: the step leaves frame 2, so the frame is not played
; (RINextAnimFrame returns 0, rows 0 and 1 stay 0).
; @fixed #1117
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-nextframe-anhd-length.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,16,16,1
r=RIAnimInit(a,0,0)
p=Peek.l(Addr BitMap(0)+8)
r=RINextAnimFrame(0)
v=Peek.w(p) AND $FFFF
w=Peek.w(p+2) AND $FFFF
FreeMem_ a,4096
NPrint "result: ",r
If w Then NPrint "frame 3's delta played" Else NPrint "frame 3's delta not played"
If v Then NPrint "frame 2's delta played" Else NPrint "frame 2's delta not played"
End
