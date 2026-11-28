; warn-datafrac - the warning datafrac (change type-system-rework, spec
; compiler/warnings): a Data.b value with a point whose fraction is not
; zero gives the warning at its line; the Byte keeps the value cut towards
; zero (-1.5 gives -1); 1.0 has no fraction and gives none. The warning is
; judged as an entry of compile.warnings with the line of this source
; (warn-datafrac.test.toml).
!TestCase{datafrac}
  Restore dfr_d
  Read a.b,b.b
  !CheckEq{a,-1,"cut"}
  !CheckEq{b,1,"whole"}
!EndTestCase
.dfr_d
Data.b -1.5,1.0
