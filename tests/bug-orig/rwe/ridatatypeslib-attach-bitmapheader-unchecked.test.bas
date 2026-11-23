; ridatatypeslib-attach-bitmapheader-unchecked (#595) - AttachDataType
; window,id takes the gadget's width and height from the BitMapHeader
; pointer that only DataTypeBitmap sets, without checking it: without
; DataTypeBitmap they are the two words at address 0 (0 here) and the
; object is attached with the size 0 x 0.  The picture (generated) is 16
; x 2 pixels; the program reads the size of the attached object (a
; gadget: Width at +8, Height at +10).
; Correct: the object is not attached with the size 0 x 0 (it gets the
; picture's size, or AttachDataType fails).
DEFTYPE .l

!TestCase{attach_size}
  WbToScreen 0
  Window 0,0,0,320,100,$100F,"DT",1,2
  opened=DataTypeOpen("CORPUS:p.ilbm")
  !Require{opened<>0,"opened"}
  o=DataTypeCreate
  !Require{o<>0,"object"}
  r=AttachDataType(0,1)
  zero=(r<>0 AND Peek.w(o+8)=0 AND Peek.w(o+10)=0)
  !CheckEq{zero,0,"attached_0x0"}
  DetachDataType
  DataTypeClose
  Free Window 0
!EndTestCase
