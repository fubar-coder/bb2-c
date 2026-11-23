; a child of rundebuglib-supertrap-d0-lost: one runtime error (ShapeWidth
; of an unused shape: "Uninitialized Shape") inside a SetInt 5 routine
; (supervisor mode)
DEFTYPE .l
NPrint "child"
SetInt 5
  If f=0
    f=1
    w=ShapeWidth(5)
  EndIf
End SetInt
For i=1 To 10 : VWait : Next
NPrint "child back"
End
