; rimisclib-progpath-empty-dir (#758) - ProgPath$ copies the program's
; directory name and decides with CMPI.b #":",-1(a3) whether to append
; "/"; with an empty name (NameFromLock fails, e.g. for a path longer
; than its buffer of 255 bytes) it looks at the byte before the result.
; The program makes a directory of a path of 279 characters in RAM: its
; program directory (SetProgramDir) for the call; the result follows
; "x" in the expression, so the byte before it is "x".
; Correct: an empty directory gives the program name alone ("x" + name).
; @rwe rimisclib
DEFTYPE .l
d$="RAM:"
k=0
For i=1 To 9
  d$=d$+String$(Chr$(96+i),30)
  If k Then UnLock_ k
  k=CreateDir_(d$)
  d$=d$+"/"
Next
If k=0 Then NPrint "no directory" : End
o=SetProgramDir_(k)
p$="x"+ProgPath$
k=SetProgramDir_(o)
UnLock_ k
NPrint p$
End
