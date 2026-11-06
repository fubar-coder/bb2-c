; rifnslib-fnsload-bank-55 (#984) - FNSLoad loads the font into
; RIAmosFuncLib bank 55 + slot, but RIAmosFuncLib has 50 banks unless
; BankLimit raised the number, and takes a number at or above the limit
; as an address: BLoad reads the file to address 55 + slot, Start(55)
; returns 0 and FNSLoad returns -1.  The font file is 1 byte, so only
; address 55 (the low byte of exception vector 13, never taken) is
; hit; a real font would overwrite the vector table and crash.
; Correct: FNSLoad does not write to address 55 (and loads the font).
; ripacklib's LoadIFF has the same mismatch (#770).
; @rwe rifnslib riamosfunclib
; @pass timeout=30
DEFTYPE .l
m=AllocMem_(16,$10001)
If m=0 Then NPrint "no memory" : End
Poke.b m,88
If WriteFile(0,"RAM:bug-orig-one.fns")
  WriteMem 0,m,1
  CloseFile 0
EndIf
Poke.b 55,0
r=FNSLoad("RAM:bug-orig-one.fns",0)
NPrint "default: ",r," ",Peek.b(55)
Poke.b 55,0
BankLimit 80
r=FNSLoad("RAM:bug-orig-one.fns",0)
NPrint "limit 80: ",r," ",Peek.b(55)
FreeMem_ m,16
End
