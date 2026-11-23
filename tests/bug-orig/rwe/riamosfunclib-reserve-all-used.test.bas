; riamosfunclib-reserve-all-used (#498) - Reserve(length) takes the bank
; from NextBank but tests the sign of the length, not NextBank's -1:
; with every bank in use it reserves bank -1.  Its "start" is the length
; of the last bank (the start table follows the length table), its
; "length" memlib's header long of the tables' block: the bank is
; "erased" (FreeMem(last length, header)) and the new block's address
; stored over the last bank's length.  Made safe: the original's
; BankLimit 2 returns the start table (#483: the port returns 2, an
; address only in the original); bank 1 is reserved with a length equal
; to the address of a chip block of the program's own whose size is that
; header long, so the FreeMem frees that block; the header is restored.
; Correct: Reserve(length) finds no free bank and leaves bank 1 alone.
DEFTYPE .l

!TestCase{reserve_all_used}
  s=BankLimit(2)
  If s>65535 Then h=Peek.l(s-12) Else h=16
  a=AllocMem_(h,2)
  !Require{a<>0,"memory"}
  r=Reserve(0,100)
  r=Reserve(1,a)
  c=Reserve(100)
  If s>65535 Then v=Peek.l(s-4) : Poke.l s-12,h Else v=Length(1)
  k=(v=a)
  If k
    FreeMem_ a,h
  Else
    If s>65535 Then Poke.l s-4,a
  EndIf
  !Check{k,"bank_1_kept"}
!EndTestCase
