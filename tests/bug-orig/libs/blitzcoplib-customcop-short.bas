; blitzcoplib-customcop-short (#404) - CustomCop copies its string as
; length/4 longs in a loop that tests the counter only at the end
; (SUBQ.l #1,d0:BNE): an empty string (or one of 1-3 bytes) gives 0 and
; the loop copies 2^32 longs over all memory behind the new copper list.
; Here: CustomCop with an empty string in Amiga mode (the list is not put
; on the hardware).  The original does not come back (crash or hang: the
; session times out).
; Correct: an empty string inserts no instruction, the program goes on.
; @configs clib
DEFTYPE .l
NPrint "before"
c$=""
CustomCop c$,100
NPrint "after"
End
