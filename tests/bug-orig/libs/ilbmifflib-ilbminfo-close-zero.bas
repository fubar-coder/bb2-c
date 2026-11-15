; ilbmifflib-ilbminfo-close-zero (#186) - ILBMInfo calls iffclose after
; ilbminit also when the file could not be opened; iffopen has then set
; the file handle to 0, and iffclose passes it to dos Close unchecked.
; Here the program patches dos.library itself (SetFunction): its Close
; (LVO -36) counts the calls with a handle of 0 (d1) and goes on to the
; original; the vector is restored afterwards. ILBMInfo of a file that
; does not exist.
; Correct: a file that was not opened is not closed - no Close(0).
; @fixed #186
; @configs clib
DEFTYPE .l
lb=OpenLibrary_("dos.library",0)
code=AllocMem_(32,$10001)
Poke.w code,$4A81                                            ; tst.l d1
Poke.w code+2,$6606                                          ; bne.s +6
Poke.w code+4,$52B9 : Poke.l code+6,code+20                  ; addq.l #1,count
Poke.w code+10,$2F39 : Poke.l code+12,code+24                ; move.l old,-(sp)
Poke.w code+16,$4E75                                         ; rts
Poke.l code+20,0 : Poke.l code+24,0                          ; count, old
Forbid_
o=SetFunction_(lb,-36,code)
Poke.l code+24,o
Permit_
r=ILBMInfo("RAM:bo186.missing")
n=Peek.l(code+20)
Forbid_
dummy=SetFunction_(lb,-36,o)
Permit_
NPrint "info ",r
NPrint "close(0) ",n
FreeMem_ code,32
CloseLibrary_ lb
End
