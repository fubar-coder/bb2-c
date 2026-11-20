; rilesdebuglib-getcustomreg-eight-chars (#712) - GetCustomReg copies the
; register's name up to its first space and stops after the 9th character
; only: a name of 8 characters (here $05A BLTCON0L) gets the first
; character of the next name written behind it, and a3 is left behind
; that character, while the length says 8.  A string expression that
; goes on behind the result builds on a3.
; Correct: "BLTCON0L" and the next part right behind it.
; @fixed #712
; @rwe rilesdebuglib
DEFTYPE .l
a$=GetCustomReg($5A)+"|"
NPrint "<",a$,">"
NPrint Len(GetCustomReg($5A))
End
