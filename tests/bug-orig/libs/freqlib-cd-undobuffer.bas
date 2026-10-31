; freqlib-cd-undobuffer (#345) - after the gadget "CD" of FileRequest$
; the requester resets the path string gadget by clearing the words at
; $111A and $111E of its data: the high word of the StringInfo's
; UndoBuffer (+4) and BufferPos (+8), meant were BufferPos and DispPos.
; UndoBuffer then points into the first 64 KB; Intuition copies the path
; there whenever the gadget is activated.
; inject clicks CD, then CANCEL (click=), and compares the UndoBuffer
; pointers of the string gadgets of the requester before the CD with
; those before the CANCEL (undobuf); the gadget is not activated (that
; would overwrite low memory).  The path is a root (RAM:), as in
; freqlib-cd-currentdir.
; Correct: CD leaves the UndoBuffer pointers alone.
; @configs clib
; @pass runwrap=@INJECT@|undobuf click=FQ/CD click=FQ/CANCEL
; @pass timeout=120
Screen 0,0,0,320,256,2,0,"freqlib",1,2
Use Screen 0
MaxLen p$=160
MaxLen f$=64
p$="RAM:"
f$="x"
r$=FileRequest$("FQ",p$,f$)
NPrint "cancel ",Len(r$)
End
