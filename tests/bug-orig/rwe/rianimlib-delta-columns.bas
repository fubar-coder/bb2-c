; rianimlib-delta-columns (#507) - the ANIM5 decoder takes the number of
; byte columns as width >> 3: a width of 12 (2 bytes per row) gives one
; column, the second is never decoded.  Frame 2 sets row 0 of both
; columns ($F0, $0F).
; Correct: both columns are decoded, the second byte of row 0 is $0F.
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-delta-columns.anim
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
v=Peek.b(p+1) AND 255
FreeMem_ a,4096
If v=$0F Then NPrint "column 1 decoded" Else NPrint "column 1 not decoded"
End
