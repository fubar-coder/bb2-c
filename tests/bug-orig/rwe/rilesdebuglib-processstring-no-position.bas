; rilesdebuglib-processstring-no-position (#719) - ProcessString works on
; the parser's current position; before D_InitParse and after the last
; D_NextArg$ the position is 0, and ProcessString reads the text at
; address 0.  The program puts "ABC" there (and puts the long back).
; Correct: without a parser position the result is empty (as D_NextArg$).
; @fixed #719
; @rwe rilesdebuglib
DEFTYPE .l
o=Peek.l(0)
Poke.l 0,$41424300
a$=ProcessString
Poke.l 0,o
NPrint "before D_InitParse: <",a$,">"
b=AllocMem_(16,$10001)
If b=0 Then NPrint "no memory" : End
Poke$ b,"x"
D_InitParse b
c$=D_NextArg$
Poke.l 0,$41424300
a$=ProcessString
Poke.l 0,o
FreeMem_ b,16
NPrint "after the last argument: <",a$,">"
End
