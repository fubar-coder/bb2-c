; chr-nospace - character literals right after the = without a space, with
; byte, word, float and a Long field as target (spec ab3-compat/strings):
; the value as with a space. AmiBlitz3 refuses these ("Can't convert
; types"; chr-nospace.test.toml).
OPTIMIZE 4
NEWTYPE .zt
  l.l
End NEWTYPE
DEFTYPE .zt t
!TestCase{chr_nospace}
  b.b=@"A"
  w.w=@"AB"
  f.f=@"AB"
  t\l=@"AB"
  !CheckEq{b,65,"l1"}
  !CheckEq{w,16706,"l2"}
  !CheckFltNear{f,16706,0.001,"l3"}
  !CheckEq{t\l,16706,"l4"}
!EndTestCase
