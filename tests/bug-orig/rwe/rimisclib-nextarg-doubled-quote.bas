; rimisclib-nextarg-doubled-quote (#755) - inside a quoted word NextArg$
; counts the quotes and ends the word at an even count unless another
; quote follows, which it copies as a literal quote.  The pair "" moves
; the count on by one only, so the closing quote behind it gives an odd
; count and NextArg$ copies the character behind it and goes on:
; "a""b" c is the one word a"b c.  At the end of the string the copied
; character is the terminating 0: "x""y" gives x"y and the 0 (4 bytes).
; Correct: "" inside quotes is one quote, the closing quote ends the
; word: a"b, c, then x"y (3 bytes), then the end (an empty string).
; @fixed #755
; @rwe rimisclib
; @pass timeout=30
DEFTYPE .l
b=AllocMem_(64,$10001)
If b=0 Then NPrint "no memory" : End
s$=Chr$(34)+"a"+Chr$(34)+Chr$(34)+"b"+Chr$(34)+" c"
For i=1 To Len(s$) : Poke.b b+i-1,Asc(Mid$(s$,i,1)) : Next
InitArgParse b
For k=1 To 3
  a$=NextArg$
  NPrint "<"+a$+"> "+Str$(Len(a$))
Next
s$=Chr$(34)+"x"+Chr$(34)+Chr$(34)+"y"+Chr$(34)
For i=1 To Len(s$) : Poke.b b+i-1,Asc(Mid$(s$,i,1)) : Next
Poke.b b+Len(s$),0
InitArgParse b
For k=1 To 2
  a$=NextArg$
  NPrint Str$(Len(a$))
Next
FreeMem_ b,64
End
