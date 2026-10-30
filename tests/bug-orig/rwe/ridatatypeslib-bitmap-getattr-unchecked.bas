; ridatatypeslib-bitmap-getattr-unchecked (#598) - DataTypeBitmap asks
; GetAttr for PDTA_BitMapHeader and takes the first long of the
; library's buffer as the BitMapHeader without looking at GetAttr's result
; or clearing the buffer first: for an object that is not a picture the
; buffer keeps what the command before left there.  The object is a text
; file (text.datatype); SetDataTypeAttr leaves its tag list in the
; buffer (DTA_TopVert, 0).
; Correct: no BitMapHeader - DataTypeBitmap returns 0.
; @rwe ridatatypeslib
; @pass file=t.txt|@TESTDATA@/ridatatypeslib-bitmap-getattr-unchecked.txt
DEFTYPE .l
If DataTypeOpen("CORPUS:t.txt")
  o=DataTypeCreate
  NPrint "object ",o<>0
  If o
    SetDataTypeAttr $80001002,0
    NPrint "bitmapheader ",Hex$(DataTypeBitmap)
  EndIf
  DataTypeClose
Else
  NPrint "not opened"
EndIf
End
