; rianimlib-chunk-signed-compare (#506) - the chunk walks compare the
; addresses signed: a chunk length that moves the pointer past
; $7FFFFFFF makes it "below" the end, and the walk goes on reading
; chunk headers from there.  The ANIM lies in Z3 fast memory
; ($40000000 on); behind its first frame a chunk claims $40000000 bytes:
; RIAnimInit's search for the second frame goes on at $80000000 and
; above (no memory there), and does not come back.
; Correct: the walk stops at the end of the ANIM, RIAnimInit returns.
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-chunk-signed-compare.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
If a<$40000000 Then NPrint "not in Z3 memory" : End
NPrint "before"
r=RIAnimInit(a,0,0)
NPrint "returned"
FreeMem_ a,4096
End
