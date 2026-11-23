; screenslib-freescreen-gfxbase (#247) - the routines that free the old
; screen of an object (Screen, ScreenTags, WbToScreen, FindScreen) call
; freescreen with a2 = their argument stack instead of the graphics
; base: if the object has a DBufInfo (ShowBitMap on V39+ before),
; FreeDBufInfo is called through garbage.
; Screen 0, ShowBitMap of a bitmap (V39+: ChangeVPBitMap, a DBufInfo for
; the object), then Screen 0 again on the same object.
; Correct: the program goes on, prints and ends.
DEFTYPE .l

!TestCase{second_screen}
  Screen 0,0,0,320,200,2,0,"first",1,2
  BitMap 0,320,200,2
  ShowBitMap 0
  Screen 0,0,0,320,200,2,0,"second",1,2
  CloseScreen 0
  !Check{1,"after"}
!EndTestCase
