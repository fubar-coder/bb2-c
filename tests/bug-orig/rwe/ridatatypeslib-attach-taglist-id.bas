; ridatatypeslib-attach-taglist-id (#596) - AttachDataType
; window,id,&taglist passes the id as the tag list of its second
; SetAttrsA and never uses the program's tag list.  The id here is 0 (an
; empty tag list in the original, so no other memory is read as tags);
; the tag list sets GA_Left ($80030001) to 5 and GA_Top ($80030003) to
; 3.  The program reads the position of the attached object (a gadget:
; LeftEdge at +4, TopEdge at +6) of the picture (generated).
; Correct: the object is at the position of the tag list (5, 3).
; @fixed #596
; The window takes the output of NPrint: DefaultOutput sends it back.
; @rwe ridatatypeslib
; @pass file=p.ilbm|@TESTDATA@/ridatatypeslib-attach-taglist-id.ilbm
DEFTYPE .l
Dim t.l(4)
t(0)=$80030001 : t(1)=5 : t(2)=$80030003 : t(3)=3 : t(4)=0
WbToScreen 0
Window 0,0,0,320,100,$100F,"DT",1,2
DefaultOutput
If DataTypeOpen("CORPUS:p.ilbm")
  o=DataTypeCreate
  NPrint "object ",o<>0
  If o
    r=AttachDataType(0,0,&t(0))
    NPrint "attached ",r<>0
    NPrint "left ",Peek.w(o+4)," top ",Peek.w(o+6)
    DetachDataType
  EndIf
  DataTypeClose
Else
  NPrint "not opened"
EndIf
End
