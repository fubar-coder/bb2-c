; rilesdebuglib-extractlabel-dot-unbounded (#703) - ExtractLabel skips 9
; characters and searches the first '.' without stopping at the 0 of the
; string: for a string without a '.' it goes on in the memory behind it.
; There the program put "xx.LEAK".
; Correct: no '.' in the string, the label is empty.
; @rwe rilesdebuglib
DEFTYPE .l
b=AllocMem_(64,$10001)
If b=0 Then NPrint "no memory" : End
Poke$ b,"no label here"
Poke$ b+14,"xx.LEAK"
a$=ExtractLabel(b)
FreeMem_ b,64
NPrint "<",a$,">"
End
