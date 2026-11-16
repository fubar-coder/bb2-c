; speaklib-translate-length (#277) - Translate$ copies the phonemes over
; its argument and counts the terminating 0 as a character of the result:
; the length is one too large, the last character is the 0, and a3 is left
; behind the 0, so in a string expression the next part follows the 0.
; Correct: Translate$("hello") is the phonemes "/HEH4LOW " (translator.
; library of the Workbench: length 9, the last character a blank, 32);
; Translate$("hello")+"x" is "/HEH4LOW x" (length 10, the 10th character
; "x" = 120, the 9th the blank).
; @fixed #277
; @configs clib
t$=Translate$("hello")
NPrint "len ",Len(t$)," last ",Asc(Right$(t$,1))
u$=Translate$("hello")+"x"
NPrint "len ",Len(u$)," 9th ",Asc(Mid$(u$,9,1))," 10th ",Asc(Mid$(u$,10,1))," last ",Asc(Right$(u$,1))
End
