; asllib-free-null-request (#403) - ASLFileRequest$/ASLPathRequest$ branch to
; filefail also when AllocAslRequest fails, and filefail always calls
; FreeFileRequest (asl LVO -36) with aslstruct - then 0.
; Here: the program patches asl.library itself (SetFunction): its
; AllocAslRequest (LVO -48) returns 0, its FreeFileRequest only records
; the requester it gets and how often it is called (no requester is
; opened, nothing is freed); both vectors are restored afterwards.
; Correct: without a requester FreeFileRequest is not called.
; @configs clib
DEFTYPE .l
MaxLen p$=160
MaxLen f$=64
p$="RAM:" : f$="x"
lb=OpenLibrary_("asl.library",0)
If lb=0 Then NPrint "no asl.library" : End
code=AllocMem_(32,$10001)
Poke.w code,$7000 : Poke.w code+2,$4E75     ; AllocAslRequest: moveq #0,d0:rts
s=code+4                                    ; FreeFileRequest:
Poke.w s,$43FA : Poke.w s+2,$000A           ;   lea data(pc),a1
Poke.w s+4,$22C8                            ;   move.l a0,(a1)+
Poke.w s+6,$5291                            ;   addq.l #1,(a1)
Poke.w s+8,$4E75 : Poke.w s+10,$4E71        ;   rts:nop
Poke.l s+12,-1 : Poke.l s+16,0              ; data: last a0, calls
Forbid_
o1=SetFunction_(lb,-48,code)
o2=SetFunction_(lb,-36,s)
Permit_
r$=ASLFileRequest$("bo403",p$,f$)
Forbid_
dummy=SetFunction_(lb,-48,o1)
dummy=SetFunction_(lb,-36,o2)
Permit_
NPrint "result ",Len(r$)
NPrint "FreeFileRequest calls ",Peek.l(s+16)
If Peek.l(s+16)>0 Then NPrint "with ",Peek.l(s+12)
FreeMem_ code,32
CloseLibrary_ lb
End
