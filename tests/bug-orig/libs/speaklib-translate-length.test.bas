; speaklib-translate-length (#277) - Translate$ copies the phonemes over
; its argument and counts the terminating 0 as a character of the result:
; the length is one too large, the last character is the 0, and a3 is left
; behind the 0, so in a string expression the next part follows the 0.
; Here: Translate$("hello") alone and in the expression
; Translate$("hello")+"x".
; Correct: Translate$("hello") is the phonemes "/HEH4LOW " (translator.
; library of the Workbench: length 9, the last character a blank, 32);
; Translate$("hello")+"x" is "/HEH4LOW x" (length 10, the 10th character
; "x" = 120, the 9th the blank).
DEFTYPE .l

!TestCase{alone}
  t$=Translate$("hello")
  l=Len(t$)
  c=Asc(Right$(t$,1))
  !CheckEq{l,9,"length"}
  !CheckEq{c,32,"last"}
!EndTestCase

!TestCase{in_expression}
  u$=Translate$("hello")+"x"
  l=Len(u$)
  a=Asc(Mid$(u$,9,1))
  b=Asc(Mid$(u$,10,1))
  c=Asc(Right$(u$,1))
  !CheckEq{l,10,"length"}
  !CheckEq{a,32,"ninth"}
  !CheckEq{b,120,"tenth"}
  !CheckEq{c,120,"last"}
!EndTestCase
