; riamosfunclib-changedir-finit-unlock (#491) - ChangeDir makes the new
; lock current and drops the directory CurrentDir returns (on the first
; call the one the program was started with); finit unlocks the last
; ChangeDir lock while it is still the current directory.  The program
; runs through inject with a lock of inject's own as its current
; directory (cd): at the end the current directory is the unlocked RAM:
; lock, not the start lock.
; Correct: the program ends in the directory it was started with.
; @rwe riamosfunclib
; @pass runwrap=@INJECT@|cd
DEFTYPE .l
r=ChangeDir("RAM:")
NPrint r
End
