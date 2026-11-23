; rimisclib-progpath-a3-behind-0 (#743) - ProgPath$, ProgDir$, ProgName$
; and DefaultFontName copy their result with the terminating 0 and leave
; a3 behind the 0 instead of on it.  In a string expression the next
; part is then written one byte too far: the result has the right length
; but holds the 0 behind the result, and the last character of the next
; part is lost.  Each result is followed by "/data"; the program checks
; the code of the last character and the number of 0 bytes.
; Correct: every result ends in "a" (97) and holds no 0 (the emulator's
; Workbench has font prefs, so DefaultFontName is called).
DEFTYPE .l
Function.l zeros{s$}
  z.l=0
  For i.l=1 To Len(s$)
    If Asc(Mid$(s$,i,1))=0 Then z=z+1
  Next
  Function Return z
End Function

!TestCase{results_plus_data}
  a$=ProgPath$+"/data"
  p1=Asc(Right$(a$,1)) : z1=zeros{a$}
  a$=ProgDir$+"/data"
  p2=Asc(Right$(a$,1)) : z2=zeros{a$}
  a$=ProgName$+"/data"
  p3=Asc(Right$(a$,1)) : z3=zeros{a$}
  o=OpenFontPrefs
  !Require{o<>0,"font_prefs"}
  a$=DefaultFontName(0)+"/data"
  p4=Asc(Right$(a$,1)) : z4=zeros{a$}
  CloseFontPrefs
  !CheckEq{p1,97,"progpath_last"}
  !CheckEq{z1,0,"progpath_zeros"}
  !CheckEq{p2,97,"progdir_last"}
  !CheckEq{z2,0,"progdir_zeros"}
  !CheckEq{p3,97,"progname_last"}
  !CheckEq{z3,0,"progname_zeros"}
  !CheckEq{p4,97,"fontname_last"}
  !CheckEq{z4,0,"fontname_zeros"}
!EndTestCase
