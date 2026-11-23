; ridatatypeslib-bitmap-getattr-unchecked (#598) - DataTypeBitmap asks
; GetAttr for PDTA_BitMapHeader and takes the first long of the
; library's buffer as the BitMapHeader without looking at GetAttr's result
; or clearing the buffer first: for an object that is not a picture the
; buffer keeps what the command before left there.  The object is a text
; file (text.datatype); SetDataTypeAttr leaves its tag list in the
; buffer (DTA_TopVert, 0).
; Correct: no BitMapHeader - DataTypeBitmap returns 0.
DEFTYPE .l

!TestCase{text_object}
  opened=DataTypeOpen("CORPUS:t.txt")
  !Require{opened<>0,"opened"}
  o=DataTypeCreate
  !Require{o<>0,"object"}
  SetDataTypeAttr $80001002,0
  bh=DataTypeBitmap
  !CheckEq{bh,0,"bitmapheader"}
  DataTypeClose
!EndTestCase
