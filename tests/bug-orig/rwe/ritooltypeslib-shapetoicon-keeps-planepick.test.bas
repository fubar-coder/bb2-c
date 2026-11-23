; ritooltypeslib-shapetoicon-keeps-planepick (#1083) - ShapeToIcon puts
; the shape into the icon's own Image (do_Gadget.GadgetRender): width,
; height, depth and data, but PlanePick and PlaneOnOff stay as the icon
; was loaded.  Intuition takes one plane of the data per bit set in
; PlanePick, so with a shape whose depth differs from the icon's image
; planes are picked that the shape does not have, or planes of the shape
; are not drawn.  Shape 0 has 2 planes, shape 1 one plane.  The program
; reads the Image's PlanePick, PlaneOnOff and Depth after ShapeToIcon 0
; into a 1-plane icon (test data ritooltypeslib-tooltypes-9.info),
; ShapeToIcon 1 into a 2-plane icon
; (ritooltypeslib-shapetoicon-keeps-planepick-2.info) and ShapeToIcon 0,1
; into the 1-plane icon (its main image; the selected image is the test
; ritooltypeslib-shapetoicon-planepick-0).
; Correct: PlanePick picks the planes of the shape ((1 << depth) - 1),
; PlaneOnOff 0.
DEFTYPE .l

!TestCase{keeps_planepick}
  BitMap 0,64,16,2
  Boxf 0,0,15,7,3
  GetaShape 0,0,0,16,8
  BitMap 1,64,16,1
  Boxf 0,0,15,7,1
  GetaShape 1,0,0,16,8
  d=GetIconObject("CORPUS:one")
  !Require{d<>0,"icon_one"}
  ShapeToIcon 0
  i=Peek.l(d+22)
  apick=Peek.b(i+14) AND 255
  aonoff=Peek.b(i+15) AND 255
  adepth=Peek.w(i+8)
  r=FreeIconObject
  d=GetIconObject("CORPUS:two")
  !Require{d<>0,"icon_two"}
  ShapeToIcon 1
  i=Peek.l(d+22)
  bpick=Peek.b(i+14) AND 255
  bonoff=Peek.b(i+15) AND 255
  bdepth=Peek.w(i+8)
  r=FreeIconObject
  d=GetIconObject("CORPUS:one")
  !Require{d<>0,"icon_one_again"}
  ShapeToIcon 0,1
  i=Peek.l(d+22)
  cpick=Peek.b(i+14) AND 255
  conoff=Peek.b(i+15) AND 255
  cdepth=Peek.w(i+8)
  r=FreeIconObject
  !CheckEq{apick,3,"a_pick"}
  !CheckEq{aonoff,0,"a_onoff"}
  !CheckEq{adepth,2,"a_depth"}
  !CheckEq{bpick,1,"b_pick"}
  !CheckEq{bonoff,0,"b_onoff"}
  !CheckEq{bdepth,1,"b_depth"}
  !CheckEq{cpick,3,"c_pick"}
  !CheckEq{conoff,0,"c_onoff"}
  !CheckEq{cdepth,2,"c_depth"}
!EndTestCase
