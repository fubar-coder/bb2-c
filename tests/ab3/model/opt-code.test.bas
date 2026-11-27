; opt-code - OPTIMIZE 4 before a statement: the code of the statement is
; AmiBlitz3's call of strings2lib's setfpumode with 0 (move.l #0,d0, a
; jsr); the words between the two labels, without the address of the jsr
; (-r: opt-code.test.toml)
!TestCase{opt_code}
  !CheckStrEq{"before","before","l1"}
optstart:
  OPTIMIZE 4
optend:
  zs$="bytes: "+Str$(?optend - ?optstart)
  !CheckStrEq{zs$,"bytes: 12","l2"}
  zs$=Hex$(Peek.w(?optstart))+" "+Hex$(Peek.l(?optstart + 2))
  zs$=zs$+" "+Hex$(Peek.w(?optstart + 6))
  !CheckStrEq{zs$,"0000203C 00000000 00004EB9","l3"}
  !CheckStrEq{"after","after","l4"}
!EndTestCase
