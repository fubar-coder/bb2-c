; riamosfunclib-reserve-all-used (#498) - Reserve(length) takes the bank
; from NextBank but tests the sign of the length, not NextBank's -1:
; with every bank in use it reserves bank -1.  Its "start" is the length
; of the last bank (the start table follows the length table), its
; "length" memlib's header long of the tables' block: the bank is
; "erased" (FreeMem(last length, header)) and the new block's address
; stored over the last bank's length.  Made safe: BankLimit 2 returns
; the start table (#483); bank 1 is reserved with a length equal to the
; address of a chip block of the program's own whose size is that header
; long, so the FreeMem frees that block; the header is restored.
; Correct: Reserve(length) finds no free bank and leaves bank 1 alone.
; @rwe riamosfunclib
DEFTYPE .l
s=BankLimit(2)
h=Peek.l(s-12)
a=AllocMem_(h,2)
If a=0 Then NPrint "no memory" : End
r=Reserve(0,100)
r=Reserve(1,a)
c=Reserve(100)
v=Peek.l(s-4)
Poke.l s-12,h
If v=a
  FreeMem_ a,h
  NPrint "bank 1 kept"
Else
  Poke.l s-4,a
  NPrint "bank 1 changed"
EndIf
End
