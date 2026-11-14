; stringfunclib-replace-empty-a3 (#424) - Replace$ with an empty find
; string returns its first argument unchanged (d0 = its length) but
; leaves a3 where the compiler left it, behind all three arguments.  In
; a string expression the next part is then written behind the
; replacement string: the result has the right length but holds the 0
; of the first argument and further argument bytes instead of the text.
; Correct: Replace$("ab","","c")+"x" is "abx" (the 3rd character "x" =
; 120), Replace$("ab","","")+"yz" is "abyz".
; @fixed #424
; @configs clib
a$=Replace$("ab","","c")+"x"
NPrint Len(a$)," ",Asc(Mid$(a$,3))
a$=Replace$("ab","","")+"yz"
NPrint Len(a$)," ",Asc(Mid$(a$,3))," ",Asc(Mid$(a$,4))
End
