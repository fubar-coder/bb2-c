; riapplib-id-range (#1097) - AddAppMenu, DelAppMenu, AddAppIcon and
; DelAppIcon use the id as the index into tables of 16 longs without a
; check; the tables lie one behind the other in one block (menus,
; windows, icons), so id 16 of AddAppMenu is window 0's slot and larger
; ids lie behind the block.  The original has no error check area.  Here
; with runtime error checking and AddAppMenu(16) (no Workbench in the
; session: AddAppMenuItemA fails, nothing is written in the original).
; Correct: the error check refuses the id: "App id must be 0 to 15".
; @fixed #1097
; @rwe riapplib
; @pass runtime_errors=on
DEFTYPE .l
NPrint "before"
r=AddAppMenu(16,"bug-orig")
NPrint "after ",r
End
