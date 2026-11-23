; ritooltypeslib-icontoshape-extra-word (#847) - IconToShape allocates
; (width + 15) >> 4 words per row for the shape's data but copies
; (width >> 4) + 1 words per row: for a width that is a multiple of 16 one
; word per row and plane more than the block holds.  The icon (SYS:Prefs)
; gets the image of a shape of 16 x 8 pixels, 1 plane (ShapeToIcon), and
; IconToShape makes shape 1 of it: 16 words copied into 8.  The program
; runs through inject (runwrap=, inject guard: a pattern behind every
; allocation, checked when it is freed).
; Correct: nothing is written behind an allocation; shape 1 is 16 x 8.
DEFTYPE .l

!TestCase{extra_word}
  BitMap 0,32,16,1
  Boxf 0,0,15,7,1
  GetaShape 0,0,0,16,8
  opened=GetIconObject("SYS:Prefs")
  !Require{opened<>0,"icon"}
  ShapeToIcon 0
  IconToShape 1
  w=ShapeWidth(1)
  h=ShapeHeight(1)
  !CheckEq{w,16,"shape_width"}
  !CheckEq{h,8,"shape_height"}
!EndTestCase
