; ridisplaylib-initview-leak (#891) - InitView keeps only the last copper
; list: every call allocates a new list (exec's AllocMem, chip memory)
; and forgets the one before; finit frees only the last.  The program
; makes ten lists and ends; it runs through inject (runwrap=, inject
; track: what the process leaves allocated).
; Correct: nothing is left allocated.
; @rwe ridisplaylib
; @pass runwrap=@INJECT@|track
DEFTYPE .l
For i=1 To 10
  c=InitView(256,5,8,32)
  If c=0 Then NPrint "no list"
Next
NPrint "lists made"
End
