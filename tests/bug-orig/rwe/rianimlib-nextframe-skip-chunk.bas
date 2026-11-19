; rianimlib-nextframe-skip-chunk (#1116) - RINextAnimFrame takes a chunk
; of the ANIM that is no FORM, between two frames, as the end of the
; animation.  The ANIM (16 x 16, 1 plane, AnimLoop Off) has an ANNO
; chunk between frame 2 (row 0 of column 0 gets $F0) and frame 3 (row 1
; gets $0F); the second RINextAnimFrame meets the ANNO and returns 0.
; Correct: the ANNO is skipped, frame 3 is played (-1, row 1 is $0F).
; @fixed #1116
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-nextframe-skip-chunk.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,16,16,1
AnimLoop Off
r=RIAnimInit(a,0,0)
p=Peek.l(Addr BitMap(0)+8)
r2=RINextAnimFrame(0)
r3=RINextAnimFrame(0)
v=Peek.b(p+2) AND 255
FreeMem_ a,4096
NPrint "frame 2: ",r2
If v<>$0F Then r3=0
If r3 Then NPrint "frame 3 played" Else NPrint "stopped at the ANNO"
End
