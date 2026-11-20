; ridisasmlib-decprint-zero-address-8 (#614) - DecPrint returns for the
; value 0 the label `.l8` where the source means its local label 'l8
; ("0",0); blitz2 2.10 assembles it as the absolute address 8, so the line
; gets the bytes of the bus error vector up to a 0 byte instead of "0":
; nothing if the vector points to ROM, other bytes if it is in RAM.  The
; test prints how many bytes stand between the name and "(a0)".
; Correct: "move.l   0(a0),d0" (a displacement 0 is "0").
; @rwe ridisasmlib
DEFTYPE .l
Dim c.w(3)
c(0)=$2028 : c(1)=0
a$=DisAssemble$(&c(0))
p=Instr(a$,"(a0),d0")
d$=Mid$(a$,10,p-10)
If d$="0"
  NPrint "displacement 0: [",a$,"]"
Else
  NPrint "displacement not 0: ",Len(d$)," bytes"
EndIf
End
