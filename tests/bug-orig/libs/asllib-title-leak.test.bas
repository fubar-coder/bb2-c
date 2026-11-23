; asllib-title-leak (#63) - every ASLFileRequest$/ASLPathRequest$ copies
; the title (and, with a pattern, the pattern) with maxslib's allocst and
; never frees the copies (no freest in asllib): each call leaks length + 5
; bytes of global memory until the program ends.  The program patches
; asl.library itself (SetFunction): its AllocAslRequest (LVO -48) returns
; 0 at once (no requester), its FreeFileRequest (LVO -36) does nothing;
; both vectors are restored afterwards.  After one call to settle the
; string variables, AvailMem is taken before and after 20 calls of
; ASLFileRequest$ with a title of 40 characters and a pattern.
; Correct: the 20 calls keep no memory (less than 100 bytes difference).
; The original keeps 20 x (45 + 11) bytes and more.
DEFTYPE .l

!TestCase{twenty_calls}
  MaxLen p$=160
  MaxLen f$=64
  p$="RAM:" : f$="x"
  t$="abcdefghijabcdefghijabcdefghijabcdefghij"
  lb=OpenLibrary_("asl.library",0)
  !Require{lb<>0,"asl_open"}
  code=AllocMem_(8,$10001)
  Poke.w code,$7000 : Poke.w code+2,$4E75     ; AllocAslRequest: moveq #0,d0:rts
  Poke.w code+4,$4E75 : Poke.w code+6,$4E71   ; FreeFileRequest: rts
  Forbid_
  o1=SetFunction_(lb,-48,code)
  o2=SetFunction_(lb,-36,code+4)
  Permit_
  r$=ASLFileRequest$(t$,p$,f$,"#?.bb2")
  m=AvailMem_(1)
  For i=1 To 20
    r$=ASLFileRequest$(t$,p$,f$,"#?.bb2")
  Next
  d=m-AvailMem_(1)
  Forbid_
  dummy=SetFunction_(lb,-48,o1)
  dummy=SetFunction_(lb,-36,o2)
  Permit_
  !Check{d<100,"no_leak"}
  FreeMem_ code,8
  CloseLibrary_ lb
!EndTestCase
