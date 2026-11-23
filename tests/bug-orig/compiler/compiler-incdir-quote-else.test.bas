; compiler-incdir-quote-else (#17) - a quoted IncDir path ends at its
; closing quote, and Else behind it ends the statement as ':' does
; (blitz2.s takes the Else into the path).  CNIF/CSIF act below all
; statements (getchar): in a false block IncDir and Include are skipped
; as any other text, the path stays and nothing is included.
; Correct: the Else branch runs, the CELSE branch runs, #k is 7.
!TestCase{incdir_else}
  a.l=0 : s1.l=0 : s2.l=0
  IncDir "CORPUS:src/"
  If a Then IncDir "CORPUS:src/" Else s1=1
  CNIF 1=0
    Include "missing.bb2"
    IncDir "CORPUS:nosuch/"
  CELSE
    s2=1
  CEND
  XInclude "inc.bb2"
  v.l=#k
  !CheckEq{s1,1,"else_branch"}
  !CheckEq{s2,1,"celse_branch"}
  !CheckEq{v,7,"constant"}
!EndTestCase
