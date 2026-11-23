; gtlib-gtshape-image (#173) - GTShape creates a GadTools button and
; copies its static image1 over the button's GadgetRender (GadTools'
; render object), and every GTShape gadget gets the one static image2 as
; SelectRender: the alternative shape of the last GTShape with two shapes
; applies to all of them.
; Here: on the Workbench screen two GTShape gadgets with two shapes each
; (0/1, then 2/3); the ImageData of their GadgetRender and SelectRender
; compared with the data of the shapes; then GTFreeGadget of the first
; and Free GTList (the gadgets with images of their own are freed).
; Correct: each gadget shows its own two shapes.  In the original the
; first gadget's SelectRender shows shape 3.
DEFTYPE .l

; the shape numbers of the render and select image of gadget n -> a, b
Statement shapes_of{n}
  SHARED a,b
  g=GTGadPtr(0,n)
  r=Peek.l(Peek.l(g+18)+10) : s=Peek.l(Peek.l(g+22)+10)
  a=-1 : b=-1
  For i=0 To 3
    If r=Peek.l(Addr Shape(i)+14) Then a=i
    If s=Peek.l(Addr Shape(i)+14) Then b=i
  Next
End Statement

!TestCase{two_gadgets}
  WbToScreen 0
  For i=0 To 3 : InitShape i,16,8,2 : Next
  GTShape 0,1,10,10,0,0,1
  GTShape 0,2,10,30,0,2,3
  shapes_of{1}
  !CheckEq{a,0,"gadget_1_render"}
  !CheckEq{b,1,"gadget_1_select"}
  shapes_of{2}
  !CheckEq{a,2,"gadget_2_render"}
  !CheckEq{b,3,"gadget_2_select"}
  GTFreeGadget 0,1
  !CheckEq{GTGadPtr(0,1),0,"gadget_1_freed"}
  Free GTList 0
!EndTestCase
