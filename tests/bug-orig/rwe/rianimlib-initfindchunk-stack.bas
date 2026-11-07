; rianimlib-initfindchunk-stack (#513) - InitFindChunk accepts a chunk
; that is no FORM but has the wanted ID at +8; in that branch it pushes
; a1 and returns without popping it: the RTS jumps to the caller's a1 -
; a crash in the original (the session ends in a timeout).  The data are
; an IFF LIST of type ANIM.  (The C port returns as if a1 had been
; popped, DIFF: it cannot reproduce the stack imbalance.)
; Correct: InitFindChunk returns; the LIST holds no FORM ILBM where the search
; looks, RIAnimDepth returns 0.
; RIPackLib calls RIAmosFuncLib.
; @fixed #513
; @rwe rianimlib ripacklib riamosfunclib
; @pass file=anim|@TESTDATA@/rianimlib-initfindchunk-stack.anim
; @pass timeout=30
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
BitMap 0,16,16,1
NPrint "before"
d.w=RIAnimDepth(a)
NPrint d
FreeMem_ a,4096
End
