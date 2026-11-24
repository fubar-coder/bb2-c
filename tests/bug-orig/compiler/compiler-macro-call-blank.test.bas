; compiler-macro-call-blank (#1207) - makename skips the blanks after the
; name of a macro call !name, and expandmax copies the rest of the line
; from the first character after them, straight behind the macro text: the
; blank between the call and what follows is lost.  A macro text ending in
; a name character is glued to a following name or digit: with the text
; !mq, the call !mw b becomes !mqb (another macro, or "Macro not Found"
; when there is none), !mw 2 becomes !mq2; a text a with !v 2 gives the
; name a2.  The macros mqb, mq2 and mnb exist here, so the originals
; compile and their glued calls give other values.
; Not affected: a text ending in another character (!mq-), an operator or
; a token after the call (!mpw + 1, !mpw MOD 3, Then), a call at the end of
; the line or before a comment, and parameters in braces, also after a
; blank (!mf {5}).
; Correct: the rest of the line follows the macro text with its blank.
DEFTYPE .l
Macro mq
10+
End Macro
Macro mqb
100
End Macro
Macro mq2
200
End Macro
Macro mw
!mq
End Macro
Macro mn
c = 10 +
End Macro
Macro mnb
c = 100
End Macro
Macro ms
!mn
End Macro
Macro mm
!mq-
End Macro
Macro mp
10
End Macro
Macro mpw
!mp
End Macro
Macro mf
`1+1
End Macro

!TestCase{glue}
  b = 2
  c = !mw b
  !CheckEq{c,12,"name"}
  c = !mw 2
  !CheckEq{c,12,"digit"}
  c = !mw   b
  !CheckEq{c,12,"blanks"}
  c = 0
  !ms b
  !CheckEq{c,12,"statement"}
!EndTestCase

!TestCase{unchanged}
  c = !mm 3
  !CheckEq{c,7,"minus_text"}
  c = !mpw + 1
  !CheckEq{c,11,"plus_after"}
  c = !mpw
  !CheckEq{c,10,"line_end"}
  c = !mpw ; a comment
  !CheckEq{c,10,"comment"}
  c = !mpw MOD 3
  !CheckEq{c,1,"mod_after"}
  d = 0
  If c = !mpw Then d = 1
  !CheckEq{d,0,"then_after"}
  c = 10
  If c = !mpw Then d = 1
  !CheckEq{d,1,"then_true"}
  c = !mf{5}
  !CheckEq{c,6,"braces"}
  c = !mf {5} * 2
  !CheckEq{c,7,"braces_blank"}
!EndTestCase
