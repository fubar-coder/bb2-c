; rianimlib-initfindchunk-stack (#513) - InitFindChunk accepts a chunk
; that is no FORM but has the wanted ID at +8; in that branch it pushes
; a1 and returns without popping it: the RTS jumps to the caller's a1.
; The data are an IFF LIST of type ANIM.  (The C port returns as if a1
; had been popped, DIFF: it cannot reproduce the stack imbalance.)
; To give the wrong return a defined end the program loads a1 with the
; address of rec before the call: the original returns to rec, which
; drops the 12 bytes the unfinished calls left on the stack (the Gosub
; frame of the driver is above them, as measured) and sets w=1.  The
; program relies on a1 being untouched by the call up to that branch and
; on the stack depth of RIAnimDepth; both are details of the library
; code, not of the bug.
; Correct: InitFindChunk returns; the LIST holds no FORM ILBM where the search
; looks, RIAnimDepth returns 0 (w stays 0).
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{initfindchunk_stack}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,16,16,1
  w=0
  LEA rec(pc),a1
  d.w=RIAnimDepth(a)
  Goto ok
rec:
  LEA 12(a7),a7
  w=1 : d=0
ok:
  !CheckEq{w,0,"return_address"}
  !CheckEq{d,0,"depth"}
  FreeMem_ a,4096
!EndTestCase
