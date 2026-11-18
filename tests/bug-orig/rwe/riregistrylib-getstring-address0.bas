; riregistrylib-getstring-address0 (#780) - GetLibraryValue$ and
; GetRegistryStringVariable take their result from the current registry
; structure (the type at 18, the string at 24) also when there is none:
; without registry.library (it exists nowhere) they read address 18 and
; 24.  Made safe: the word at 18 and the long at 24 (part of the exception
; vectors) are set to 1 (a string) and to a string "OLD" with interrupts
; disabled for the two calls and restored at once.
; Correct: without a structure both return "" and read nothing.
; @fixed #780
; @rwe riregistrylib
m.l=AllocMem_(8,$10001)
Poke.b m,79 : Poke.b m+1,76 : Poke.b m+2,68
NPrint "open ",IsRegistryOpen
Disable_
ow.w=Peek.w(18) : ol.l=Peek.l(24)
Poke.w 18,1 : Poke.l 24,m
a$=GetLibraryValue$("x")
b$=GetRegistryStringVariable("y")
Poke.w 18,ow : Poke.l 24,ol
Enable_
NPrint "[",a$,"]"
NPrint "[",b$,"]"
FreeMem_ m,8
End
