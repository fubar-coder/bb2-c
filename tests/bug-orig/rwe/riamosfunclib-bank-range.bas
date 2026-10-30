; riamosfunclib-bank-range (#484) - the bank routines only test
; bank >= maxbanks (signed): a negative bank number counts as a bank and
; indexes before the tables.  The start table directly follows the
; length table, so Start(-1) returns the length of the last bank (49 of
; the default 50), here 100.
; Correct: bank -1 is no bank, Start(-1) returns 0.
; @rwe riamosfunclib
DEFTYPE .l
r=Reserve(49,100)
NPrint Start(-1)
End
