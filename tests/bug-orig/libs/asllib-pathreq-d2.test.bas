; asllib-pathreq-d2 (#61) - ASLPathRequest$ uses the code of
; ASLFileRequest$ and takes d2 as the file name variable, although it has
; no such argument: after OK it copies fr_File to the address in d2 and
; writes its length to d2-4.  With dimensions d2 is x: x = 20 writes the
; length to $10 and the name to $14 (the exception vectors "Illegal
; Instruction" and "Zero Divide").  The program patches asl.library
; itself (SetFunction): AllocAslRequest (LVO -48) returns a requester of
; its own (fr_File "f", fr_Drawer "RAM:"), AslRequest (LVO -60) returns
; TRUE at once, FreeFileRequest (LVO -36) does nothing; the vectors are
; restored afterwards.  The longs at $10 and $14 are saved before the call
; and compared after it (under Forbid, restored at once).
; Correct: the vectors are not touched; the result is "RAM:f" and the
; path "RAM:".  The original overwrites them.
DEFTYPE .l

!TestCase{path_request_vectors}
  MaxLen p$=160
  p$="SYS:"
  lb=OpenLibrary_("asl.library",0)
  !Require{lb<>0,"asl_open"}
  code=AllocMem_(64,$10001)
  Poke.w code,$41FA : Poke.w code+2,$000E     ; AllocAslRequest: lea req(pc),a0
  Poke.w code+4,$2008 : Poke.w code+6,$4E75   ;   move.l a0,d0:rts
  Poke.w code+8,$7001 : Poke.w code+10,$4E75  ; AslRequest: moveq #1,d0:rts
  Poke.w code+12,$4E75 : Poke.w code+14,$4E71 ; FreeFileRequest: rts
  Poke.l code+20,code+40                      ; req: fr_File
  Poke.l code+24,code+44                      ;      fr_Drawer
  Poke.b code+40,Asc("f")
  Poke.b code+44,Asc("R") : Poke.b code+45,Asc("A")
  Poke.b code+46,Asc("M") : Poke.b code+47,Asc(":")
  Forbid_
  o1=SetFunction_(lb,-48,code)
  o2=SetFunction_(lb,-60,code+8)
  o3=SetFunction_(lb,-36,code+12)
  Permit_
  Forbid_
  v10=Peek.l($10) : v14=Peek.l($14)
  r$=ASLPathRequest$("bo61",p$,20,20,280,160)
  w10=Peek.l($10) : w14=Peek.l($14)
  Poke.l $10,v10 : Poke.l $14,v14
  Permit_
  Forbid_
  dummy=SetFunction_(lb,-48,o1)
  dummy=SetFunction_(lb,-60,o2)
  dummy=SetFunction_(lb,-36,o3)
  Permit_
  !CheckStrEq{r$,"RAM:f","result"}
  !CheckStrEq{p$,"RAM:","path"}
  !Check{w10=v10 AND w14=v14,"vectors_kept"}
  FreeMem_ code,64
  CloseLibrary_ lb
!EndTestCase
