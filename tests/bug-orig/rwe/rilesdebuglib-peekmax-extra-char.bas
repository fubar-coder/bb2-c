; rilesdebuglib-peekmax-extra-char (#718) - PeekMax$ tests the count
; after the copy: it writes max+1 characters and returns the length max,
; a3 is left one character behind the result.  A string expression that
; goes on behind the result builds on a3.
; Correct: PeekMax$(&t$,4) is "abcd", and the next part right behind it;
; a max below 1 gives the empty string.
; @fixed #718
; @rwe rilesdebuglib
DEFTYPE .l
t$="abcdefgh"
a$=PeekMax$(&t$,4)+"|"
NPrint "<",a$,">"
a$=PeekMax$(&t$,0)+"|"
NPrint "<",a$,">"
End
