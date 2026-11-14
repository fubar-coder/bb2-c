; datelib-numdays-short-field (#1029) - the day and month fields of
; NumDays read their second character and the "/" after it without a
; test for the end of the string: a short string ("", "1", "1/", "1/1")
; reads the 0 byte as a digit and the bytes behind it.  When "/" follows,
; the routine parses on into the memory behind the string and returns a
; date made of whatever is there.
; Here the empty string, with known bytes behind it: the assignment of
; "99/05/1990" right before leaves that text in the string work area, and
; the empty argument puts only its 0 byte over the first "9": the day
; field reads 0 and "9", then "/05/1990".  (The shorter strings "1" and
; "1/" depend on more of the work area, which the original and the C
; libraries leave differently; the month field's reads give an invalid
; month after #124 anyway.)
; Correct: a field that ends at the end of the string is invalid input:
; 0; a complete date is unchanged.
; @fixed #1029
; @configs clib
DEFTYPE .l
a$="99/05/1990" : n=NumDays("") : NPrint "[] ",n
NPrint "[1/1/1990] ",NumDays("1/1/1990")
End
