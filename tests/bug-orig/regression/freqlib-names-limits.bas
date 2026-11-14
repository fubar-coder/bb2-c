; freqlib-names-limits - the fix of #1042: FileRequest$ takes what fits its
; buffers (159 / 63 characters) and the MaxLen of Pathname$ and Filename$,
; and refuses the rest (freqlib-names-refused: the clicked file names).
; The drawer CORPUS:fqname holds a file of 28 characters; a line shows
; its first 27 characters (320 pixel screen).  inject clicks (click=, one
; window title per call):
;   F1  MaxLen f$=30: the 28-character name, then OK: taken
;   F2  a path of 159 characters ending in "/", PARENT (which adds a "/"),
;       then OK: refused, the path keeps its 159 characters
;   F3  a start name of 64 characters (MaxLen f$=64), then OK: the name is
;       not taken, the file field is empty
; Without the fix F2 writes the 0 behind the path buffer (into the FILE:
; gadget) and F3 copies without end at OK (a crash).
; @pass dir=fqname
; @pass runwrap=@INJECT@|click=F1/m123456789m123456789m123456 click=F1/OK click=F2/PARENT click=F2/OK click=F3/OK
; @pass timeout=120
Screen 0,0,0,320,256,2,0,"freqlib",1,2
Use Screen 0
If WriteFile(0,"CORPUS:fqname/m123456789m123456789m1234567") Then CloseFile 0
MaxLen p$=160
MaxLen f$=30
p$="CORPUS:fqname"
f$="y"
r$=FileRequest$("F1",p$,f$)
NPrint "F1 ",Len(r$)," file ",Len(f$)," [",f$,"]"
p$="RAM:"+String$("a",154)+"/"
f$="x"
r$=FileRequest$("F2",p$,f$)
NPrint "F2 ",Len(r$)," path ",Len(p$)," ",Right$(p$,3)
MaxLen f$=64
p$="RAM:"
f$=String$("z",64)
r$=FileRequest$("F3",p$,f$)
NPrint "F3 ",Len(r$)," [",r$,"] file ",Len(f$)
End
