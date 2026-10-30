; rianimlib-delta-count-zero (#508) - the vertical delta decoders loop
; over the count of a "same" op (0, count, value) with SUBQ.b #1 : DBRA
; on the byte: a count of 0 runs 256 times.  Frame 2 of the ANIM
; (16 x 16, ANIM5) has a run of count 0 in column 0; the bitmap has 300
; rows, so the 256 rows stay inside its plane; row 100 shows the run.
; Correct: a run of count 0 writes no row, row 100 stays 0.
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-delta-count-zero.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,16,300,1
r=RIAnimInit(a,0,0)
p=Peek.l(Addr BitMap(0)+8)
r=RINextAnimFrame(0)
v=Peek.b(p+200) AND 255
FreeMem_ a,4096
If v Then NPrint "row 100 written" Else NPrint "row 100 untouched"
End
