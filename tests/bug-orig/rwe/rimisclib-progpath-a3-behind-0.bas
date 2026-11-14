; rimisclib-progpath-a3-behind-0 (#743) - ProgPath$, ProgDir$, ProgName$
; and DefaultFontName copy their result with the terminating 0 and leave
; a3 behind the 0 instead of on it.  In a string expression the next
; part is then written one byte too far: the result has the right length
; but holds the 0 behind the result, and the last character of the next
; part is lost.  Each result is followed by "/data"; the program prints
; the code of the last character and the number of 0 bytes.
; Correct: every result ends in "a" (97) and holds no 0 (the emulator's
; Workbench has font prefs; without them DefaultFontName is not called).
; @fixed #743
; @rwe rimisclib
DEFTYPE .l
Function.l zeros{s$}
  z.l=0
  For i.l=1 To Len(s$)
    If Asc(Mid$(s$,i,1))=0 Then z=z+1
  Next
  Function Return z
End Function
a$=ProgPath$+"/data"
NPrint "ProgPath$: ",Asc(Right$(a$,1))," ",zeros{a$}
a$=ProgDir$+"/data"
NPrint "ProgDir$: ",Asc(Right$(a$,1))," ",zeros{a$}
a$=ProgName$+"/data"
NPrint "ProgName$: ",Asc(Right$(a$,1))," ",zeros{a$}
If OpenFontPrefs
  a$=DefaultFontName(0)+"/data"
  If Len(a$)>5 Then NPrint "DefaultFontName: ",Asc(Right$(a$,1))," ",zeros{a$}
  CloseFontPrefs
EndIf
End
