; bb2 - blitz2-ab3's BB2 mode from the command line (source-mode = "bb2";
; spec ab3-compat/source-mode): what stays blitz2's there
Macro m
If `1
End
End If
zs$="after"
End Macro
!TestCase{ptr_bb2}
  ; a word value assigned to a pointer is converted as in blitz2 (the type
  ; rule of AB3 mode is not applied)
  w.w = 4 : *p.l = w
  !CheckEq{*p,4,"l1"}
!EndTestCase
!TestCase{macend_bb2}
  ; a macro line ending in End keeps the rest of the macro, as in AB3 mode
  ; (Blitz Basic 2 loses it, "If Without End If" at the call; fixed, #968)
  zs$="" : !m{0}
  !CheckStrEq{zs$,"after","l1"}
  !CheckStrEq{"done","done","l2"}
!EndTestCase
!TestCase{asm_short_s_bb2}
  ; the size suffix .s of an assembler instruction is .b, and a branch with
  ; .s or .b is a short branch, with .w a word branch, as in blitz2
  ; (extension #1075; AB3 mode keeps AmiBlitz3's word branch, case
  ; asm_short_s): BEQ.s is taken, BRA.s is 2 bytes ($6000+displacement),
  ; BRA.w 4 bytes, MOVE.s is MOVE.b
  MOVEQ #5,d0
  CMP.l #5,d0
  BEQ.s ok
  MOVEQ #0,d0
ok:
  PutReg d0,a.l
  !CheckEq{a,5,"l1"}
l1:
  BRA.s l2
  MOVEQ #0,d0
l2:
  BRA.w l3
l3:
  zs$=Str$(?l2 - ?l1)+" "+Hex$(Peek.w(?l1) AND $FFFF)+" "+Str$(?l3 - ?l2)
  !CheckStrEq{zs$,"4 00006002 4","l2"}
  MOVEQ #-1,d0
  MOVE.s #5,d0
  PutReg d0,a
  !CheckEq{a,-251,"l3"}
!EndTestCase
