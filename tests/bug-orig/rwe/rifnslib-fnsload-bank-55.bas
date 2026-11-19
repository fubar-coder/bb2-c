; rifnslib-fnsload-bank-55 (#984) - FNSLoad loads the font into
; RIAmosFuncLib bank 55 + slot, but RIAmosFuncLib has 50 banks unless
; BankLimit raised the number, and takes a number at or above the limit
; as an address: BLoad reads the file to address 55 + slot, Start(55)
; returns 0 and FNSLoad returns -1.  The font file is a font header of
; 12 bytes ("FNS.", height 8, width 16, underline 6, 16 bytes per
; character), so only the addresses 55 to 66 are hit (exception vectors
; 13 to 16, never taken; the program restores them); a whole font would
; overwrite the vector table and crash.
; Correct: FNSLoad does not write to address 55 and loads the font (the
; slot, 0, in both lines; the byte at 55 stays 0).
; ripacklib's LoadIFF has the same mismatch (#770).
; @fixed #984
; @rwe rifnslib riamosfunclib
; @pass timeout=30
DEFTYPE .l
m=AllocMem_(32,$10001)
v=AllocMem_(16,$10001)
If m=0 OR v=0 Then NPrint "no memory" : End
Poke.l m,$464E532E
Poke.w m+4,8 : Poke.w m+6,16 : Poke.w m+8,6 : Poke.w m+10,16
If WriteFile(0,"RAM:bug-orig-head.fns")
  WriteMem 0,m,12
  CloseFile 0
EndIf
For i=0 To 11 : Poke.b v+i,Peek.b(55+i) : Next
Poke.b 55,0
r=FNSLoad("RAM:bug-orig-head.fns",0)
b=Peek.b(55)
For i=0 To 11 : Poke.b 55+i,Peek.b(v+i) : Next
NPrint "default: ",r," ",b
Poke.b 55,0
BankLimit 80
r=FNSLoad("RAM:bug-orig-head.fns",0)
b=Peek.b(55)
For i=0 To 11 : Poke.b 55+i,Peek.b(v+i) : Next
NPrint "limit 80: ",r," ",b
FreeMem_ v,16
FreeMem_ m,32
End
