; riamosfunclib-eraseall-dbra (#495) - EraseAll counts from maxbanks - 1
; down with DBRA, which tests only the low word, but uses the whole long
; as the bank number: with BankLimit 65537 it frees bank 65536 and stops
; when the low word wraps; bank 0 stays allocated.  (BankLimit 0 runs the
; loop over 65536 negative banks before the tables - a crash.)
; Correct: EraseAll frees every bank, Start(0) is 0 afterwards.
; @rwe riamosfunclib
DEFTYPE .l
s=BankLimit(65537)
r=Reserve(0,16)
r=Reserve(65536,16)
EraseAll
If Start(0)=0 Then NPrint "bank 0 freed" Else NPrint "bank 0 left"
End
