; t57_ferrin - fileiolib: input from a closed file (runtime error of the
; input handler, from the normal code)
NPrint "open ",WriteFile(0,"RAM:t57.dat")
FileInput 0
CloseFile 0
a$=Inkey$
DefaultInput
NPrint "after [",a$,"]"
End
