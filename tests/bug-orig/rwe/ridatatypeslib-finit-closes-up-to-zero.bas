; ridatatypeslib-finit-closes-up-to-zero (#602) - init opens
; datatypes.library, dos.library, intuition.library and graphics.library
; into a table and stores 0 for one that does not open; finit closes the
; table's entries up to the first 0.  The program runs through inject
; with datatypes.library (the first entry) failing to open (runwrap=,
; inject openlibfail; opencount: how often the process opened minus
; closed the other three; the names given by their endings).
; Correct: every library that was opened is closed again (+0).
; @rwe ridatatypeslib
; @pass runwrap=@INJECT@|openlibfail=types.library opencount=dos.library opencount=tion.library opencount=hics.library
NPrint "open ",DataTypeOpen("CORPUS:nothing")
End
