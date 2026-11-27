; opt-020 - OPTIMIZE 5 (68020 + new syntax): compiled with 68000 code (the
; 68020 bit is accepted, docs/ab3/variant.md), the results are AmiBlitz3's
OPTIMIZE 5
!TestCase{opt_020}
  Dim zarr.l(100)
  For i.l = 0 To 100
    zarr(i) = i * i * 1000
  Next
  x.l = 123456
  y.l = -789
  zs$=Str$(x * y)+" "+Str$(x / y)+" "+Str$(x MOD 1000)
  !CheckStrEq{zs$,"-97406784 -156 456","l1"}
  zs$=Str$(zarr(100))+" "+Str$(zarr(37) / 7)
  !CheckStrEq{zs$,"10000000 195571","l2"}
!EndTestCase
