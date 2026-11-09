; rundebuglib-printerr-numeric-code (#6) - an executable compiled with
; runtime error checking gets rundebuglib instead of debuglib: its
; printerr takes d0 of the TRAP #0 as the message text, also for the
; numeric codes 1-3 that only debuglib translates.  The requester shows
; an empty text (the byte at address 1, 2 or 3).  Code 1 is the used
; object test (RGB without a screen), code 2 the bounds check of an
; array field of a NewType (v\x[i]), code 3 the end of a program without
; End (blitz2's eopcode).  The program runs itself twice (own name from
; dos GetProgramName, dos Execute, output to the same file), with 1 and
; 2 for the first two codes; then it runs off its end.
; Correct: the texts of debuglib ("No currently used object!", "Offset
; out of range", "End of Program").
; @fixed #6
; @pass runtime_errors=on
DEFTYPE .l
NEWTYPE .t
  x.w[4]
End NEWTYPE
DEFTYPE .t v
If NumPars=0
  nm$=String$(Chr$(0),256) : GetProgramName_ &nm$,255
  n$=Peek$(&nm$)
  c$=n$+" 1" : Execute_ &c$,0,Output_
  c$=n$+" 2" : Execute_ &c$,0,Output_
  NPrint "parent"
Else
  NPrint "child ",Par$(1)
  If Par$(1)="1" Then RGB 1,15,15,15
  i=Len(Par$(1))+5 : v\x[i]=1
  NPrint "child not stopped"
  End
EndIf
