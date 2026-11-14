; datelib-numdays-short-year (#125) - the year field of NumDays ends only
; at a "/" or after 4 characters, not at the end of the string: a year of
; fewer than 4 digits without a "/" behind it ("1/1/78") reads the 0 byte
; at the end of the string and the bytes behind it as further digits
; (character - 48), and an empty year ("1/1/") starts with them.
; Here NumDays of years of 2 and 3 digits without a "/" behind them,
; compared with the same date with the "/" (-1 = the same), and whether
; an empty year gives 0.  (Compared, not printed: the counts of years
; before 1900 differ from the original's by the Gregorian rule, #1001.)
; Correct: the year ends at the end of the string, "1/1/78" is the same
; as "1/1/78/" (the year 78); an empty year is invalid input (0).  The
; original's results depend on the memory behind the string.
; @fixed #125
; @configs clib
DEFTYPE .l
NPrint "[1/1/78] ",NumDays("1/1/78")=NumDays("1/1/78/")
NPrint "[1/1/999] ",NumDays("1/1/999")=NumDays("1/1/999/")
NPrint "[1/1/1978] ",NumDays("1/1/1978")=NumDays("1/1/1978/")
NPrint "[1/1/] ",NumDays("1/1/")=0
End
