; ritooltypeslib-shapetoicon-keeps-planepick (#1083) - ShapeToIcon puts
; the shape into the icon's own Image (do_Gadget.GadgetRender): width,
; height, depth and data, but PlanePick and PlaneOnOff stay as the icon
; was loaded.  Intuition takes one plane of the data per bit set in
; PlanePick, so with a shape whose depth differs from the icon's image
; planes are picked that the shape does not have, or planes of the shape
; are not drawn.  Shape 0 has 2 planes, shape 1 one plane.  The program
; prints the Image's PlanePick, PlaneOnOff and Depth after ShapeToIcon 0
; into a 1-plane icon (test data ritooltypeslib-tooltypes-9.info),
; ShapeToIcon 1 into a 2-plane icon
; (ritooltypeslib-shapetoicon-keeps-planepick-2.info) and ShapeToIcon 0,1
; into the 1-plane icon (its main image; the selected image is the test
; ritooltypeslib-shapetoicon-planepick-0).
; Correct: PlanePick picks the planes of the shape ((1 << depth) - 1),
; PlaneOnOff 0.
; @fixed #1083
; @rwe ritooltypeslib
; @pass file=one.info|@TESTDATA@/ritooltypeslib-tooltypes-9.info
; @pass file=two.info|@TESTDATA@/ritooltypeslib-shapetoicon-keeps-planepick-2.info
DEFTYPE .l
BitMap 0,64,16,2
Boxf 0,0,15,7,3
GetaShape 0,0,0,16,8
BitMap 1,64,16,1
Boxf 0,0,15,7,1
GetaShape 1,0,0,16,8
d=GetIconObject("CORPUS:one")
If d=0 Then NPrint "no icon" : End
ShapeToIcon 0
i=Peek.l(d+22)
NPrint "A pick ",Peek.b(i+14)&255," onoff ",Peek.b(i+15)&255," depth ",Peek.w(i+8)
FreeIconObject
d=GetIconObject("CORPUS:two")
If d=0 Then NPrint "no icon" : End
ShapeToIcon 1
i=Peek.l(d+22)
NPrint "B pick ",Peek.b(i+14)&255," onoff ",Peek.b(i+15)&255," depth ",Peek.w(i+8)
FreeIconObject
d=GetIconObject("CORPUS:one")
If d=0 Then NPrint "no icon" : End
ShapeToIcon 0,1
i=Peek.l(d+22)
NPrint "C pick ",Peek.b(i+14)&255," onoff ",Peek.b(i+15)&255," depth ",Peek.w(i+8)
FreeIconObject
End
