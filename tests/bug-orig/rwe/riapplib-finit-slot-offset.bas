; riapplib-finit-slot-offset (#524) - at program end delapp clears the
; table slot of each object left with MOVE.l #0,0(a2,d7.l) where d7 is the
; slot index, not index * 4: for slot 1 an odd address, an address error
; on a 68000 (the session ends in a timeout).  The machine is the A600
; (68000).  The sessions run no Workbench, so AddAppMenu would fail: the
; program patches workbench.library itself (SetFunction): its
; AddAppMenuItemA (LVO -72) returns a fake item ($100), its
; RemoveAppMenuItem (LVO -78) returns 0 and does nothing.  The App menu
; item of slot 1 is left to finit, which calls RemoveAppMenuItem and
; clears the slot; the vectors stay patched (finit runs after the
; program's last line), the code is not freed.
; Correct: the program ends normally.
; @fixed #524
; @rwe riapplib
; @machine a600
; @pass timeout=30
DEFTYPE .l
wb=OpenLibrary_("workbench.library",37)
If wb=0 Then NPrint "no workbench.library" : End
code=AllocMem_(16,$10001)
If code=0 Then NPrint "no memory" : End
Poke.w code,$203C : Poke.l code+2,$100      ; AddAppMenuItemA: move.l #$100,d0
Poke.w code+6,$4E75                         ;   rts
Poke.w code+8,$7000 : Poke.w code+10,$4E75  ; RemoveAppMenuItem: moveq #0,d0:rts
Forbid_
dummy=SetFunction_(wb,-72,code)
dummy=SetFunction_(wb,-78,code+8)
Permit_
CloseLibrary_ wb
If AddAppMenu(1,"bug-orig") Then NPrint "menu added" Else NPrint "no menu"
End
