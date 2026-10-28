; t77_menuerr - menuslib error check itemchk2: MenuItem of a menu that
; does not exist
MenuTitle 0,0,"One"
MenuItem 0,0,0,0,"Item"
NPrint "before"
MenuItem 0,0,2,0,"Wrong"
NPrint "after"
End
