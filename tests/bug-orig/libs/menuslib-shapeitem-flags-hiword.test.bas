; menuslib-shapeitem-flags-hiword (#218) - ShapeItem and ShapeSub fetch
; their Flags argument (a pushed long) with MOVE 4(a7),d7: its HIGH word,
; which sussflags then reads as a long behind the cleared low word of
; mexclude.  Small flag values (1-4: toggle, exclude, checked) give the
; default flags ($52, without ITEMTEXT $50).  The Flags word (+12) and
; MutualExclude (+14) of a shape item with flags 1 and one with flags 2
; (item 2: all others excluded), and of a shape subitem with flags 3.
; Correct: as MenuItem with the same flags, without ITEMTEXT: $59, $59
; with MutualExclude $FFFFFFFB, and $159.
DEFTYPE .l

!TestCase{shape_flags}
  InitShape 0,16,8,1
  MenuTitle 0,0,"Project"
  ShapeItem 0,1,0,0,0
  MenuItem 0,0,0,1,"Sub"
  ShapeItem 0,2,0,2,0
  ShapeSub 0,3,0,1,0,0
  m=Peek.l(Addr MenuList(0))
  i0=Peek.l(m+18) : i1=Peek.l(i0) : i2=Peek.l(i1) : s0=Peek.l(i1+28)
  !CheckEq{Peek.w(i0+12),$59,"item_0_flags"}
  !CheckEq{Peek.l(i0+14),0,"item_0_exclude"}
  !CheckEq{Peek.w(i2+12),$59,"item_2_flags"}
  !CheckEq{Peek.l(i2+14),$FFFFFFFB,"item_2_exclude"}
  !CheckEq{Peek.w(s0+12),$159,"sub_0_flags"}
!EndTestCase
