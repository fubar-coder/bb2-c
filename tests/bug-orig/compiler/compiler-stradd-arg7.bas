; compiler-stradd-arg7 (#971) - a string addition evaluated at d6 (regat
; 6: an argument from the 7th on that is passed in a register, or an
; addition nested that deep) gets a wrong length: getelement saves d6 on
; the stack around the right string and its restore pops the right
; string's length instead; addstrings then pops the saved d6 and adds it
; to the left length, the right length is lost.
; Library commands with a string there are rare (AddBox's text$, the 8th
; argument; Screen's and Window's titles are not: their pushed arguments
; take no register), so the test nests the addition: the parentheses of
; the 7th level evaluate at d6.
; Correct: the lengths 15, 16, 17 and the whole string.
; @fixed #971
t$="a"+("b"+("c"+("d"+("e"+("f"+"123456789")))))
NPrint "six levels ",Len(t$)
t$="a"+("b"+("c"+("d"+("e"+("f"+("g"+"123456789"))))))
NPrint "seven levels ",Len(t$)
t$="a"+("b"+("c"+("d"+("e"+("f"+("g"+("h"+"123456789")))))))
NPrint "eight levels ",Len(t$)
NPrint t$
End
