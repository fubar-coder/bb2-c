; ritooltypeslib-shapetoicon-planepick-0 (#854) - ShapeToIcon shape,shape
; puts the second shape into the library's own Image for the icon's
; selected image, whose PlanePick and PlaneOnOff stay 0: no plane of the
; image is picked.  The shapes have 2 planes; the program reads the
; selected Image of the open icon (SYS:Prefs): PlanePick, PlaneOnOff and
; its depth, and the gadget flags (6, GADGHIMAGE: documented, no defect).
; Correct: PlanePick picks the planes of the shape (3), PlaneOnOff 0.
DEFTYPE .l

!TestCase{selected_image}
  BitMap 0,64,16,2
  Boxf 0,0,19,7,3
  GetaShape 0,0,0,20,8
  GetaShape 1,0,0,20,8
  d=GetIconObject("SYS:Prefs")
  !Require{d<>0,"icon"}
  ShapeToIcon 0,1
  i=Peek.l(d+26)
  pick=Peek.b(i+14) AND 255
  onoff=Peek.b(i+15) AND 255
  depth=Peek.w(i+8)
  flags=Peek.w(d+16)
  r=FreeIconObject
  !CheckEq{pick,3,"pick"}
  !CheckEq{onoff,0,"onoff"}
  !CheckEq{depth,2,"depth"}
  !CheckEq{flags,6,"flags"}
!EndTestCase
