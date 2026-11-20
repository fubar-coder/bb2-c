; rimapeditorlib-mapobjname-byte-length (#899) - MapObjName$ copies all
; characters of the map's name but counts them with ADDQ.b: the length
; of the result is the count modulo 256.  A name of 300 characters comes
; back with the length 44.
; Correct: the name comes back whole, 300 characters.
; @fixed #899
; @rwe rimapeditorlib
CreateMap 0,4,4,16,16,"m"
SetMapObjName$ String$("a",300)
a$=MapObjName$
NPrint Len(a$)
If a$=String$("a",300) Then NPrint "name whole"
End
