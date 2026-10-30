; rilesdebuglib-initparse-overflow (#716) - D_InitParse copies the string
; with its 0 into its 256-byte buffer without a limit; the parser's
; pointer curadr lies behind the buffer.  A string of 259 characters puts
; its last three and the 0 into curadr: the program chooses them so that
; curadr points to "hello" in a block of its own (an address whose low
; byte is 0 and whose other bytes are neither 0 nor ","), and D_NextArg$
; returns that text.  (Longer strings overwrite the code behind it.)
; Correct: D_InitParse copies at most 255 characters, the first argument
; is the start of the string.
; @rwe rilesdebuglib
DEFTYPE .l
b=AllocMem_(131072,$10005)
If b=0 Then NPrint "no memory" : End
x=0
For y=(b+1024) AND $FFFFFF00 To b+131072-256 Step 256
  If x=0
    c1=(y LSR 24) AND 255 : c2=(y LSR 16) AND 255 : c3=(y LSR 8) AND 255
    If c1<>0 AND c1<>44 AND c2<>0 AND c2<>44 AND c3<>0 AND c3<>44 Then x=y
  EndIf
Next
If x=0 Then NPrint "no address" : End
For i=0 To 255 : Poke.b b+i,97 : Next
Poke.b b+256,c1 : Poke.b b+257,c2 : Poke.b b+258,c3 : Poke.b b+259,0
Poke$ x,"hello"
D_InitParse b
a$=D_NextArg$
FreeMem_ b,131072
If a$="hello" Then NPrint "pointer overwritten"
If Left$(a$,10)="aaaaaaaaaa" Then NPrint "string"
End
