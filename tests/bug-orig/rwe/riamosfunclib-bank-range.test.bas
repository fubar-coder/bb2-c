; riamosfunclib-bank-range (#484) - the bank routines only test
; bank >= maxbanks (signed): a negative bank number counts as a bank and
; indexes before the tables.  The start table directly follows the
; length table, so Start(-1) returns the length of the last bank (49 of
; the default 50), here 100.  Reserve(bank,length) does not test the bank
; at all: Reserve(50,16) stores the new block's address behind the start
; table and the length over the start of bank 0, and returns -1.
; Made safe: the original's BankLimit 50 returns the start table (#483:
; the port returns 50, the guard s>65535), the long behind
; it is read before the Reserve and written back right after it under
; Forbid, the start of bank 0 is cleared again and the block freed.
; Correct: bank -1 is no bank, Start(-1) returns 0; Reserve(50,16) is
; outside the 50 banks and returns 0.
DEFTYPE .l

!TestCase{bank_range}
  s=BankLimit(50)
  r=Reserve(49,100)
  x1=Start(-1)
  Erase 49
  If s>65535 Then h=Peek.l(s+200)
  Forbid_
  r=Reserve(50,16)
  If s>65535
    a=Peek.l(s+200)
    If a<>h Then Poke.l s+200,h : Poke.l s,0 : FreeMem_ a,16
  EndIf
  Permit_
  !CheckEq{x1,0,"start_minus_1"}
  !CheckEq{r,0,"reserve_50"}
!EndTestCase
