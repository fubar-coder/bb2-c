; rilesdebuglib-fprint-long-string (#710) - FPrint takes the length of the
; string as the DBRA counter with SUBQ.w #1 : BMI 'exit: a length of
; 32769 to 65536 is a negative word, nothing is printed.  The string
; buffer is raised (tedstub strbuf=) for the string of 40000 characters;
; FPrint draws into plane 0 of the used bitmap (640 pixels wide, rows of
; 80 bytes): the first character's 8 bytes at the cursor 0,0.
; Correct: FPrint prints the first 80 characters.
; @rwe rilesdebuglib
; @pass jobline=strbuf=150000
DEFTYPE .l
BitMap 0,640,16,1
a$=String$("x",40000)
FLocate 0,0
FPrint a$
p=Peek.l(Addr BitMap(0)+8)
k=0
For i=0 To 7 : k+Peek.b(p+i*80) AND 255 : Next
If k Then NPrint "printed" Else NPrint "nothing printed"
End
