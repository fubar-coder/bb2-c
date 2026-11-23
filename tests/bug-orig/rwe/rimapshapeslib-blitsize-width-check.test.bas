; rimapshapeslib-blitsize-width-check (#906) - BlitMapShapeSize limits the
; height to the map shape's, not the width: a width above the map shape's
; makes BltBitMapRastPort read each row on into the next rows, and the
; last row behind the shape's data.  Here with runtime error checking: a
; 16x16 map shape blitted 64 pixels wide into a window on the Workbench
; screen (the blit only reads behind the data).
; Correct: the error check refuses the width: "BlitMapShapeSize wider
; than the map shape".
DEFTYPE .l

!TestCase{wide_refused}
  BitMap 0,320,64,1
  Boxf 0,0,15,15,1
  GetaShape 0,0,0,16,16
  SetNumMapShapes 10
  ShapeToMapShape 0,0
  WbToScreen 0
  Window 0,20,30,200,80,$000E,"BlitMapShapeSize",1,2
  BlitMapShapeSize 0,0,20,20,16,16
  !Check{1,"sixteen_wide"}
  !ExpectError{"BlitMapShapeSize wider than the map shape","refused"}
  BlitMapShapeSize 0,0,20,20,64,16
  !Check{0,"sixtyfour_wide"}
!EndTestCase
