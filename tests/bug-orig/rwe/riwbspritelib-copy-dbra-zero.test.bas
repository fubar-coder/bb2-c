; riwbspritelib-copy-dbra-zero (#917) - GetWBSprite and ChangeWBSprite
; copy the shape with height - 1 and depth - 1 as DBRA counts: a shape
; object of height or depth 0 would copy 65536 rows or planes.  Blitz's
; own shape commands make no such shape: an invalid object (here a shape
; whose height word is set to 0).  The release code does not reach the
; copy for height 0 (AllocBitMap returns 0, GetWBSprite fails), and
; AllocBitMap itself does not return for depth 0; the original has no
; error check area.  Here with runtime error checking.
; Correct: the error check refuses the shape before AllocBitMap: "Illegal
; Shape Height".
DEFTYPE .l

!TestCase{height_zero}
  BitMap 0,320,64,2
  Boxf 0,0,15,15,3
  GetaShape 0,0,0,16,16
  Poke.w Addr Shape(0)+2,0
  !ExpectError{"Illegal Shape Height","refused"}
  s=GetWBSprite(10,20,0,2)
  !CheckEq{s,-1,"no_sprite"}
!EndTestCase
