; proc-defaults - an optional parameter of each type left out gets its
; default: -1, quick and float -1.0 (spec ab3-compat/procedures). The case
; prints the values (asserted in proc-defaults.test.toml): Print's form of
; the Quick and Float defaults is the subject.
OPTIMIZE 4
Statement s{x.l,@b.b,@w.w,@l.l,@q.q,@f.f}
  NPrint x," ",b," ",w," ",l," ",q," ",f
End Statement
!TestCase{proc_defaults}
  s{1}
  s{2,3,4,5,6.5,7.5}
  NPrint "end"
!EndTestCase
