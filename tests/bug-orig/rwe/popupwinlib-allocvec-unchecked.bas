; popupwinlib-allocvec-unchecked (#467) - each menu level allocates its
; 64-byte structure with AllocVec and does not check the result: with a
; failed allocation the structure is at address 0 - the item list is
; read from address 10, the sizes and the window data are written to
; addresses 14-39 (exception vectors), a window is opened for the
; garbage list and waits for a mouse button (the session ends in a
; timeout).  The program runs through inject with the first allocation
; of 68 bytes failing (AllocVec(64): 64 + its size field).
; Correct: PopUpWindow fails without a window and returns, the program
; goes on.
; @rwe popupwinlib
; @pass runwrap=@INJECT@|allocfail=68-68/1
; @pass timeout=30
DEFTYPE .l
it=AllocMem_(64,$10001)
If it=0 Then NPrint "no memory" : End
Poke.l it+32,$4974656D
Poke.l it+4,it+32
WbToScreen 0
NPrint "before"
PopUpWindow 0,it
NPrint "after"
FreeMem_ it,64
End
