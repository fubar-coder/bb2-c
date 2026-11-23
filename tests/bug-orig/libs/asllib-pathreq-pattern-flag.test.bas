; asllib-pathreq-pattern-flag (#64) - ASLPathRequest$ without dimensions
; does not reset the tag ASLFR_Flags1 (FRF_DOPATTERNS): after an
; ASLFileRequest$ with a pattern the path requester shows the pattern
; gadget too.  The program patches asl.library itself (SetFunction): its
; AllocAslRequest (LVO -48) records the data of the tag ASLFR_Flags1
; ($80080014) of the tag list it gets and returns 0 (no requester), its
; FreeFileRequest (LVO -36) does nothing; both vectors are restored
; afterwards.  First ASLFileRequest$ with a pattern, then ASLPathRequest$
; without dimensions.
; Correct: the file requester with the pattern gets the flags 1, the
; path requester 0.  The original gives the path requester 1 as well.
DEFTYPE .l

!TestCase{pattern_flag}
  MaxLen p$=160
  MaxLen f$=64
  p$="RAM:" : f$="x"
  lb=OpenLibrary_("asl.library",0)
  !Require{lb<>0,"asl_open"}
  code=AllocMem_(32,$10001)
  Poke.w code,$43FA : Poke.w code+2,$0016     ; lea data(pc),a1
  Poke.w code+4,$2218                         ; loop: move.l (a0)+,d1
  Poke.w code+6,$670C                         ;   beq.s done
  Poke.w code+8,$2018                         ;   move.l (a0)+,d0
  Poke.w code+10,$0C81 : Poke.l code+12,$80080014 ; cmp.l #ASLFR_Flags1,d1
  Poke.w code+16,$66F2                        ;   bne.s loop
  Poke.w code+18,$2280                        ;   move.l d0,(a1)
  Poke.w code+20,$7000 : Poke.w code+22,$4E75 ; done: moveq #0,d0:rts
  Poke.l code+24,-1                           ; data: the flags
  Poke.w code+28,$4E75                        ; FreeFileRequest: rts
  Forbid_
  o1=SetFunction_(lb,-48,code)
  o2=SetFunction_(lb,-36,code+28)
  Permit_
  r$=ASLFileRequest$("bo64",p$,f$,"#?.bb2")
  fl1=Peek.l(code+24)
  Poke.l code+24,-1
  r$=ASLPathRequest$("bo64",p$)
  fl2=Peek.l(code+24)
  Forbid_
  dummy=SetFunction_(lb,-48,o1)
  dummy=SetFunction_(lb,-36,o2)
  Permit_
  !CheckEq{fl1,1,"file_requester_flags"}
  !CheckEq{fl2,0,"path_requester_flags"}
  FreeMem_ code,32
  CloseLibrary_ lb
!EndTestCase
