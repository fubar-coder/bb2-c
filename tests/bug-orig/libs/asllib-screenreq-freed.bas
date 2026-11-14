; asllib-screenreq-freed (#60) - ASLScreenRequest frees the screen mode
; requester (asl LVO -36) and then returns its address: the program reads
; the chosen mode from freed memory (as the manual's example does).  The
; program patches asl.library itself (SetFunction): AllocAslRequest (LVO
; -48) returns a requester of its own with a chosen mode (ID $12345678,
; 640 x 256, depth 3, bitmap 704 x 280), AslRequest (LVO -60) returns
; TRUE at once, and FreeFileRequest (LVO -36) overwrites the first 16
; bytes of the requester it frees with -1 (as a reuse of freed memory
; would); the vectors are restored afterwards.  The result is read as
; the manual's NEWTYPE .screeninfo (id.l width.l height.l depth.w
; overscan.w autoscroll.w bmapwidth.l bmapheight.l).
; Correct: the values read through the result are the chosen ones.  The
; original returns the overwritten requester.
; @fixed #60
; @configs clib
DEFTYPE .l
lb=OpenLibrary_("asl.library",0)
If lb=0 Then NPrint "no asl.library" : End
code=AllocMem_(96,$10001)
Poke.w code,$41FA : Poke.w code+2,$001E     ; AllocAslRequest: lea req(pc),a0
Poke.w code+4,$2008 : Poke.w code+6,$4E75   ;   move.l a0,d0:rts
Poke.w code+8,$7001 : Poke.w code+10,$4E75  ; AslRequest: moveq #1,d0:rts
Poke.w code+12,$70FF                        ; FreeFileRequest: moveq #-1,d0
Poke.w code+14,$20C0 : Poke.w code+16,$20C0 ;   4 x move.l d0,(a0)+
Poke.w code+18,$20C0 : Poke.w code+20,$20C0
Poke.w code+22,$4E75                        ;   rts
Poke.l code+32,$12345678                    ; req: sm_DisplayID
Poke.l code+36,640 : Poke.l code+40,256     ;      width, height
Poke.w code+44,3 : Poke.w code+46,1         ;      depth, overscan
Poke.w code+48,-1                           ;      autoscroll
Poke.l code+50,704 : Poke.l code+54,280     ;      bitmap width, height
FindScreen 0
Forbid_
o1=SetFunction_(lb,-48,code)
o2=SetFunction_(lb,-60,code+8)
o3=SetFunction_(lb,-36,code+12)
Permit_
s=ASLScreenRequest(31)
Forbid_
dummy=SetFunction_(lb,-48,o1)
dummy=SetFunction_(lb,-60,o2)
dummy=SetFunction_(lb,-36,o3)
Permit_
If s=0 Then NPrint "cancelled" : End
Print Hex$(Peek.l(s))," ",Peek.l(s+4)," ",Peek.l(s+8)," ",Peek.w(s+12)," "
NPrint Peek.w(s+14)," ",Peek.w(s+16)," ",Peek.l(s+18)," ",Peek.l(s+22)
FreeMem_ code,96
CloseLibrary_ lb
End
